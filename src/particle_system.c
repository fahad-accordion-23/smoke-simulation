#include "particle_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* CONSTANTS */

#define SPAWN_RATE 1000

#define GRAVITY +9.81f
#define PI 3.141592654f

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

#define PARTICLE_MASS 1000.0f
#define FLUID_DENSITY 1562.5f
#define SPEED_SOUND 20.0f
#define VISCOSITY 1000.0f

#define POLY6_CONSTANT (4.0f / (PI * CUTOFF_DISTANCE_TO_8))

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

void PS_init(ParticleSystem *sys, Arena *arena, size_t num_particles, int bound_x, int bound_y) {
    sys->max_particles   = num_particles;
    sys->alive_particles = 0;
    sys->bound_x         = bound_x;
    sys->bound_y         = bound_y;

    sys->density   = arena_alloc(arena, sizeof(float)   , num_particles);
    sys->pressure  = arena_alloc(arena, sizeof(float)   , num_particles);
    sys->force     = arena_alloc(arena, sizeof(Vector2f), num_particles);
    sys->acc       = arena_alloc(arena, sizeof(Vector2f), num_particles);
    sys->vel       = arena_alloc(arena, sizeof(Vector2f), num_particles);
    sys->pos       = arena_alloc(arena, sizeof(Vector2f), num_particles);
    sys->color     = arena_alloc(arena, sizeof(Color)   , num_particles);

    memset(sys->density, 0, sizeof(float) * num_particles);
}

void PS_generate_random_particles(ParticleSystem *sys, float dt) {
    if (sys->alive_particles >= sys->max_particles)
        return; 

    size_t batch_size = (size_t) ((float) SPAWN_RATE * dt);

    if (sys->alive_particles + batch_size > sys->max_particles)
        batch_size = sys->max_particles - sys->alive_particles;

    size_t start_idx = sys->alive_particles;
    size_t final_idx = sys->alive_particles + batch_size;

    for (size_t i = start_idx; i < final_idx; i++) {
        sys->pos[i].x = (float) sys->bound_x / 2.0f + f_generate_rand(-12.5f, 12.5f);
        sys->pos[i].y = (float) sys->bound_y / 2.0f + f_generate_rand(-12.5f, 12.5f);
    }

    for (size_t i = start_idx; i< final_idx; i++) {
        sys->color[i].value = 0xFFFFFFFF;
    }

    sys->alive_particles += batch_size;
}

void PS_tick(ParticleSystem *sys, float dt) {
    memset(sys->density, 0, sizeof(float)    * sys->max_particles);
    memset(sys->acc    , 0, sizeof(Vector2f) * sys->max_particles);

    // calculating densities
    for (size_t i = 0; i < sys->alive_particles; i++) {
        float density = 0;

        for (size_t j = 0; j < sys->alive_particles; j++) {
            float weight = poly6_kernel(sys->pos[i], sys->pos[j]);
            density += PARTICLE_MASS * weight;
        }

        sys->density[i] = density;
    }

    // calculating pressures
    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->pressure[i] = (SPEED_SOUND * SPEED_SOUND) * (sys->density[i] - FLUID_DENSITY);
    }

    // calculating accelerations
    for (size_t i = 0; i < sys->alive_particles; i++) {
        float density_i         = sys->density[i];
        float density_i_squared = density_i * density_i;

        Vector2f pos_i          = sys->pos[i];
        Vector2f acc_i          = { 0 };

        for (size_t j = 0; j < sys->alive_particles; j++) {
            if (i == j)
                continue;

            float density_j         = sys->density[j];
            float density_j_squared = density_j * density_j;

            Vector2f pos_j          = sys->pos[j];

            float dist_squared      = get_dist_squared(pos_i, pos_j);
            float diff              = get_diff(dist_squared);

            // acceleration due to pressure
            {
                float term_l = sys->pressure[i] / density_i_squared;
                float term_r = sys->pressure[j] / density_j_squared;

                float P_ij = -(PARTICLE_MASS / density_j) * (term_l + term_r);

                float gradient_x = POLY6_CONSTANT * (-6.0f * diff * diff * (pos_i.x - pos_j.x));
                float gradient_y = POLY6_CONSTANT * (-6.0f * diff * diff * (pos_i.y - pos_j.y));

                acc_i.x += P_ij * gradient_x;
                acc_i.y += P_ij * gradient_y;
            }

            // acceleration due to viscosity
            {
                Vector2f term_l;
                term_l.x = sys->vel[i].x / density_i_squared;
                term_l.y = sys->vel[i].y / density_i_squared;

                Vector2f term_r;
                term_r.x = sys->vel[j].x / density_j_squared;
                term_r.y = sys->vel[j].y / density_j_squared;

                Vector2f V_ij;
                V_ij.x = -VISCOSITY * (PARTICLE_MASS / sys->density[j]) * (term_l.x + term_r.x);
                V_ij.y = -VISCOSITY * (PARTICLE_MASS / sys->density[j]) * (term_l.y + term_r.y);

                float laplacian = POLY6_CONSTANT * (24.0f * dist_squared * diff - 12.0f * diff * diff);

                acc_i.x += V_ij.x * laplacian;
                acc_i.y += V_ij.y * laplacian;
            }
        }

        // acceleration due to external forces
        acc_i.x += (1.0f / density_i) * sys->force[i].x;
        acc_i.y += (1.0f / density_i) * sys->force[i].y;

        // acceleration due to gravity
        acc_i.y += GRAVITY;

        sys->acc[i] = acc_i;
    }

    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->vel[i].x += sys->acc[i].x * dt;
        sys->vel[i].y += sys->acc[i].y * dt;
    }

    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->pos[i].x += sys->vel[i].x * dt;
        sys->pos[i].y += sys->vel[i].y * dt;
   }
}
