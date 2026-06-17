#include "particle_system.h"
#include <stdlib.h>
#include <string.h>

/* CONSTANTS */

#define SPAWN_RATE 1000

#define GRAVITY       +9.81f
#define PI            3.141592654f
#define PARTICLE_MASS 1000000.0f
#define FLUID_DENSITY 1562000.5f
#define SPEED_SOUND   20.0f
#define VISCOSITY     1000000.0f

#define CUTOFF_DISTANCE 5.0f
#define CUTOFF_DISTANCE_SQUARED CUTOFF_DISTANCE * CUTOFF_DISTANCE
#define CUTOFF_DISTANCE_TO_8 (CUTOFF_DISTANCE * \
                              CUTOFF_DISTANCE * \
                              CUTOFF_DISTANCE * \
                              CUTOFF_DISTANCE * \
                              CUTOFF_DISTANCE * \
                              CUTOFF_DISTANCE * \
                              CUTOFF_DISTANCE * \
                              CUTOFF_DISTANCE)
#define POLY6_CONSTANT (4.0f / (PI * CUTOFF_DISTANCE_TO_8))

/* PRIVATE HELPERS */

float f_generate_rand (float min, float max);
float get_dist_squared(Vector2f r_i, Vector2f r_j);
float get_diff        (float dist_squared);
float poly6_kernel    (Vector2f r_i, Vector2f r_j);
size_t get_cell_index (Vector2f position, Vector2i cell_dimensions, size_t cell_cols);

void sort_particles         (ParticleSystem *sys);
void calculate_densities    (ParticleSystem *sys);
void calculate_pressures    (ParticleSystem *sys);
void calculate_accelerations(ParticleSystem *sys);
void calculate_velocities   (ParticleSystem *sys, float dt);
void calculate_positions    (ParticleSystem *sys, float dt);

/* ACTUAL IMPLEMENTATION */

void PS_init(ParticleSystem *sys,
             Arena *arena,
             size_t max_particles,
             Vector2i dimensions,
             Vector2i cell_dimensions) {

    sys->max_particles   = max_particles;

    sys->dimensions = dimensions;

    sys->cell_dimensions = cell_dimensions;
 
    sys->inset.x = cell_dimensions.x * 5;
    sys->inset.y = cell_dimensions.y * 5;

    sys->cell_cols = (size_t) (dimensions.x / cell_dimensions.x);
    sys->cell_rows = (size_t) (dimensions.y / cell_dimensions.y);

    sys->bin = arena_alloc(arena, sizeof(Bin), sys->cell_rows * sys->cell_cols);

    sys->density = arena_alloc(arena, sizeof(float), max_particles);

    sys->pressure = arena_alloc(arena, sizeof(float), max_particles);

    sys->force   = arena_alloc(arena, sizeof(Vector2f), max_particles);
    sys->force_s = arena_alloc(arena, sizeof(Vector2f), max_particles);

    sys->acc = arena_alloc(arena, sizeof(Vector2f), max_particles);

    sys->vel   = arena_alloc(arena, sizeof(Vector2f), max_particles);
    sys->vel_s = arena_alloc(arena, sizeof(Vector2f), max_particles);

    sys->pos   = arena_alloc(arena, sizeof(Vector2f), max_particles);
    sys->pos_s = arena_alloc(arena, sizeof(Vector2f), max_particles);

    sys->color   = arena_alloc(arena, sizeof(Color), max_particles);
    sys->color_s = arena_alloc(arena, sizeof(Color), max_particles);

    sys->num_boundary_particles = (size_t) (sys->dimensions.x * 2 + sys->dimensions.y * 2 - 4);
    sys->pos_b = arena_alloc(arena, sizeof(Vector2f), sys->num_boundary_particles);

    memset(sys->density, 0, sizeof(float) * max_particles);

    memset(sys->force  , 0, sizeof(Vector2f) * max_particles);
    memset(sys->force_s, 0, sizeof(Vector2f) * max_particles);

    memset(sys->vel   , 0, sizeof(Vector2f) * max_particles);
    memset(sys->vel_s , 0, sizeof(Vector2f) * max_particles);

    memset(sys->bin, 0, sizeof(Bin) * sys->cell_rows * sys->cell_cols);
}

void PS_generate_boundary_particles(ParticleSystem *sys) {
    Vector2i inset = sys->inset;

    int width  = sys->dimensions.x;
    int height = sys->dimensions.y;

    int next_particle_index = 0;

    for (int i = inset.x; i < width - inset.x; i++) {
        sys->pos_b[next_particle_index].x = (float) i;
        sys->pos_b[next_particle_index].y = (float) inset.y;
        next_particle_index++;
    }

    for (int i = inset.x; i < width - inset.x; i++) {
        sys->pos_b[next_particle_index].x = (float) i;
        sys->pos_b[next_particle_index].y = (float) (height - inset.y - 1);
        next_particle_index++;
    }

    for (int i = inset.y; i < height - inset.y - 1; i++) {
        sys->pos_b[next_particle_index].x = (float) inset.x;
        sys->pos_b[next_particle_index].y = (float) i;
        next_particle_index++;
    }

    for (int i = inset.y; i < height - inset.y - 1; i++) {
        sys->pos_b[next_particle_index].x = (float) (width - inset.x - 1);
        sys->pos_b[next_particle_index].y = (float) i;
        next_particle_index++;
    }
}

void PS_generate_random_particles(ParticleSystem *sys) {
    for (size_t i = 0; i < sys->max_particles; i++) {

        float min_x = (2.0f / 3.0f) * (float) (sys->dimensions.x - sys->inset.x);
        float max_x = (float) (sys->dimensions.x - sys->inset.x - sys->cell_dimensions.x);

        float min_y = (float) (sys->dimensions.y / 2);
        float max_y = (float) (sys->dimensions.y - sys->inset.y - sys->cell_dimensions.y);

        sys->pos[i].x = f_generate_rand(min_x, max_x);
        sys->pos[i].y = f_generate_rand(min_y, max_y);
    }

    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->color[i].value = 0xFFFFFFFF;
    }
}

void PS_tick(ParticleSystem *sys, float dt) {
    sort_particles(sys);
    calculate_densities(sys);
    calculate_pressures(sys);
    calculate_accelerations(sys);
    calculate_velocities(sys, dt);
    calculate_positions(sys, dt);
}

/* HELPERS IMPLEMENTATION */

float f_generate_rand(float min, float max) {
    return ((float) rand() / (float) RAND_MAX) * (max - min) + min;
}

float get_dist_squared(Vector2f r_i, Vector2f r_j) {
    float dx = r_i.x - r_j.x;
    float dy = r_i.y - r_j.y;

    return dx * dx + dy * dy;
}

float get_diff(float dist_squared) {
    if (dist_squared > CUTOFF_DISTANCE_SQUARED)
        return 0.0f;

    return CUTOFF_DISTANCE_SQUARED - dist_squared;
}

float poly6_kernel(Vector2f r_i, Vector2f r_j) {
    float dist_squared = get_dist_squared(r_i, r_j);
    float diff         = get_diff(dist_squared);

    return POLY6_CONSTANT * diff * diff * diff;
}

size_t get_cell_index(Vector2f position, Vector2i cell_dimensions, size_t cell_cols) {
    size_t index;

    index  = (size_t) ((int) position.y / cell_dimensions.y) * cell_cols;
    index += (size_t) ((int) position.x / cell_dimensions.x);

    return index;
}

void sort_particles(ParticleSystem *sys) {
    size_t   max_particles   = sys->max_particles;
    Vector2i cell_dimensions = sys->cell_dimensions;
    size_t   cell_rows       = sys->cell_rows;
    size_t   cell_cols       = sys->cell_cols;

    memset(sys->bin, 0, sizeof(Bin) * cell_cols * cell_rows);

    // counting the particles
    for (size_t i = 0; i < max_particles; i++) {
        size_t bin_index = get_cell_index(sys->pos[i], cell_dimensions, cell_cols);

        sys->bin[bin_index].start_index += 1;
        sys->bin[bin_index].count       += 1;
    }

    // prefix sum calculation
    for (size_t i = 1; i < cell_rows * cell_cols; i++) {
        sys->bin[i].start_index += sys->bin[i - 1].start_index;
    }

    // sorting
    for (size_t i = 0; i < max_particles; i++) {

        size_t bin_index = get_cell_index(sys->pos[i], cell_dimensions, cell_cols);
        Bin    *bin      = &sys->bin[bin_index];

        size_t new_index = bin->start_index - 1;

        sys->force_s[new_index] = sys->force[i];
        sys->vel_s  [new_index] = sys->vel  [i];
        sys->pos_s  [new_index] = sys->pos  [i];
        sys->color_s[new_index] = sys->color[i];

        bin->start_index -= 1;
    }

    void *temp; 

    temp         = (void*) sys->force;
    sys->force   = sys->force_s;
    sys->force_s = (Vector2f*) temp;

    temp       = (void*) sys->vel;
    sys->vel   = sys->vel_s;
    sys->vel_s = (Vector2f*) temp;

    temp       = (void*) sys->pos;
    sys->pos   = sys->pos_s;
    sys->pos_s = (Vector2f*) temp;

    temp         = (void*) sys->color;
    sys->color   = sys->color_s;
    sys->color_s = (Color*) temp;
}

void calculate_densities(ParticleSystem *sys) {
    memset(sys->density, 0, sizeof(float)    * sys->max_particles);

    size_t max_particles     = sys->max_particles;
    Vector2i cell_dimensions = sys->cell_dimensions;
    size_t cell_rows         = sys->cell_rows;
    size_t cell_cols         = sys->cell_cols;

    for (size_t i = 0; i < max_particles; i++) {
        Vector2f pos_i   = sys->pos[i];
        float    density = 0;

        int cell_x = (int) pos_i.x / cell_dimensions.x;
        int cell_y = (int) pos_i.y / cell_dimensions.y;

        for (int dx = -1; dx <= 1; dx++) {
            int neighbour_x = cell_x + dx;

            if (neighbour_x < 0 || (size_t) neighbour_x >= cell_cols)
                continue;

            for (int dy = -1; dy <= 1; dy++) {
                int neighbour_y = cell_y + dy;
                
                if (neighbour_y < 0 || (size_t) neighbour_y >= cell_rows)
                    continue;

                size_t bin_index = (size_t) neighbour_y * cell_cols + (size_t) neighbour_x;
                Bin    *bin      = &sys->bin[bin_index];

                size_t start_idx = bin->start_index;
                size_t final_idx = bin->start_index + bin->count;

                for (size_t j = start_idx; j < final_idx; j++) {
                    float weight = poly6_kernel(sys->pos[i], sys->pos[j]);
                    density += PARTICLE_MASS * weight;
                }
            }
        }

        sys->density[i] = density;
    }
}

void calculate_pressures(ParticleSystem *sys) {
    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->pressure[i] = (SPEED_SOUND * SPEED_SOUND) * (sys->density[i] - FLUID_DENSITY);
    }
}

void calculate_accelerations(ParticleSystem *sys) {
    size_t max_particles     = sys->max_particles;
    Vector2i cell_dimensions = sys->cell_dimensions;
    size_t cell_rows         = sys->cell_rows;
    size_t cell_cols         = sys->cell_cols;

    for (size_t i = 0; i < max_particles; i++) {

        float    density_i  = sys->density[i];
        float    pressure_i = sys->pressure[i];
        Vector2f acc_i      = { 0 };
        Vector2f vel_i      = sys->vel[i];
        Vector2f pos_i      = sys->pos[i];

        float density_i_squared = density_i * density_i;

        int cell_x = (int) pos_i.x / cell_dimensions.x;
        int cell_y = (int) pos_i.y / cell_dimensions.y;

        for (int dx = -1; dx <= 1; dx++) {
            int neighbour_x = cell_x + dx;

            if (neighbour_x < 0 || (size_t) neighbour_x >= cell_cols)
                continue;

            for (int dy = -1; dy <= 1; dy++) {
                int neighbour_y = cell_y + dy;
                
                if (neighbour_y < 0 || (size_t) neighbour_y >= cell_rows)
                    continue;

                size_t bin_index = (size_t) neighbour_y * cell_cols + (size_t) neighbour_x;
                Bin    *bin      = &sys->bin[bin_index];

                size_t start_idx = bin->start_index;
                size_t final_idx = bin->start_index + bin->count;

                for (size_t j = start_idx; j < final_idx; j++) {
                    if (i == j)
                        continue;

                    float    density_j  = sys->density[j];
                    float    pressure_j = sys->pressure[j];
                    Vector2f vel_j      = sys->vel[j];
                    Vector2f pos_j      = sys->pos[j];

                    float density_j_squared = density_j * density_j;
                    float dist_squared      = get_dist_squared(pos_i, pos_j);
                    float diff              = get_diff(dist_squared);

                    // acceleration due to pressure
                    {
                        float term_l = pressure_i / density_i_squared;
                        float term_r = pressure_j / density_j_squared;

                        float P_ij = -(PARTICLE_MASS / density_j) * (term_l + term_r);

                        float gradient_x = POLY6_CONSTANT * (-6.0f * diff * diff * (pos_i.x - pos_j.x));
                        float gradient_y = POLY6_CONSTANT * (-6.0f * diff * diff * (pos_i.y - pos_j.y));

                        acc_i.x += P_ij * gradient_x;
                        acc_i.y += P_ij * gradient_y;
                    }

                    // acceleration due to viscosity
                    {
                        Vector2f term_l;
                        term_l.x = vel_i.x / density_i_squared;
                        term_l.y = vel_i.y / density_i_squared;

                        Vector2f term_r;
                        term_r.x = vel_j.x / density_j_squared;
                        term_r.y = vel_j.y / density_j_squared;

                        Vector2f V_ij;
                        V_ij.x = -VISCOSITY * (PARTICLE_MASS / density_j) * (term_l.x + term_r.x);
                        V_ij.y = -VISCOSITY * (PARTICLE_MASS / density_j) * (term_l.y + term_r.y);

                        float laplacian;
                        laplacian = POLY6_CONSTANT * (24.0f * dist_squared * diff - 12.0f * diff * diff);

                        acc_i.x += V_ij.x * laplacian;
                        acc_i.y += V_ij.y * laplacian;
                    }
                }
            }
        }

        acc_i.x += (1.0f / density_i) * sys->force[i].x;
        acc_i.y += (1.0f / density_i) * sys->force[i].y;

        // acceleration due to gravity
        acc_i.y += GRAVITY;

        sys->acc[i] = acc_i;
    }
}

void calculate_velocities(ParticleSystem *sys, float dt) {
    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->vel[i].x += sys->acc[i].x * dt;
        sys->vel[i].y += sys->acc[i].y * dt;
    }
}

void calculate_positions(ParticleSystem *sys, float dt) {
    Vector2i dimen = sys->dimensions;
    Vector2i inset = sys->inset;

    for (size_t i = 0; i < sys->max_particles; i++) {
        Vector2f pos = sys->pos[i];
        Vector2f vel = sys->vel[i];
        
        float dx = vel.x * dt;
        float dy = vel.y * dt;

        pos.x += dx;
        pos.y += dy;

        if ((float) (0 + inset.x) >= pos.x || pos.x >= (float) (dimen.x - inset.x))
        {
            pos.x -= dx;
            sys->vel[i].x *= -0.80f;
        }

        if ((float) (0 + inset.y) >= pos.y || pos.y >= (float) (dimen.y - inset.y))
        {
            pos.y -= dy;
            sys->vel[i].y *= -0.80f;
        }

        sys->pos[i] = pos;
   }
}
