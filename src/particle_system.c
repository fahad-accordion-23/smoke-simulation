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

float get_dist_squared(float x, float x_j, float y, float y_j) {
    float dx = x - x_j;
    float dy = y - y_j;

    return dx * dx + dy * dy;
}

float get_diff(float dist_squared) {
    if (dist_squared > CUTOFF_DISTANCE_SQUARED)
        return 0.0f;

    return CUTOFF_DISTANCE_SQUARED - dist_squared;
}

float poly6_kernel(float x, float x_j, float y, float y_j) {
    float dist_squared = get_dist_squared(x, x_j, y, y_j);
    float diff         = get_diff(dist_squared);

    return POLY6_CONSTANT * diff * diff * diff;
}

void PS_init(ParticleSystem *sys, Arena *arena, size_t num_particles, int bound_x, int bound_y) {
    sys->max_particles   = num_particles;
    sys->alive_particles = 0;
    sys->bound_x         = bound_x;
    sys->bound_y         = bound_y;
    sys->accumulated_dt  = 0.0f;

    sys->density   = arena_alloc(arena, sizeof(float), num_particles);
    sys->pressure  = arena_alloc(arena, sizeof(float), num_particles);
    sys->fx        = arena_alloc(arena, sizeof(float), num_particles);
    sys->fy        = arena_alloc(arena, sizeof(float), num_particles);
    sys->ax        = arena_alloc(arena, sizeof(float), num_particles);
    sys->ay        = arena_alloc(arena, sizeof(float), num_particles);
    sys->vx        = arena_alloc(arena, sizeof(float), num_particles);
    sys->vy        = arena_alloc(arena, sizeof(float), num_particles);
    sys->x         = arena_alloc(arena, sizeof(float), num_particles);
    sys->y         = arena_alloc(arena, sizeof(float), num_particles);
    sys->color     = arena_alloc(arena, sizeof(Color), num_particles);

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
        sys->x[i] = (float) sys->bound_x / 2.0f + f_generate_rand(-12.5f, 12.5f);
    }

    for (size_t i = start_idx; i < final_idx; i++) {
        sys->y[i] = (float) sys->bound_y / 2.0f + f_generate_rand(-12.5f, 12.5f);
    }

    for (size_t i = start_idx; i< final_idx; i++) {
        sys->color[i].value = 0xFFFFFFFF;
    }

    sys->alive_particles += batch_size;
}

void PS_tick(ParticleSystem *sys, float dt) {
    // calculating densities
    memset(sys->density, 0, sizeof(float) * sys->max_particles);
    memset(sys->ax, 0, sizeof(float) * sys->max_particles);
    memset(sys->ay, 0, sizeof(float) * sys->max_particles);

    for (size_t i = 0; i < sys->alive_particles; i++) {
        float density = 0;

        for (size_t j = 0; j < sys->alive_particles; j++) {
            float weight = poly6_kernel(sys->x[i], sys->x[j], sys->y[i], sys->y[j]);
            density += PARTICLE_MASS * weight;
        }

        sys->density[i] = density;
    }

    // calculating pressures
    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->pressure[i] = (SPEED_SOUND * SPEED_SOUND) * (sys->density[i] - FLUID_DENSITY);
    }

    // acceleration due to pressure
    for (size_t i = 0; i < sys->alive_particles; i++) {
        for (size_t j = 0; j < sys->alive_particles; j++) {
            if (i == j)
                continue;

            float density_i_squared = sys->density[i] * sys->density[i];
            float density_j_squared = sys->density[j] * sys->density[j];
            float dist_squared      = get_dist_squared(sys->x[i], sys->x[j], sys->y[i], sys->y[j]);
            float diff              = get_diff(dist_squared);

            // acceleration due to pressure
            {
                float term_l = sys->pressure[i] / density_i_squared;
                float term_r = sys->pressure[j] / density_j_squared;

                float P_ij = -(PARTICLE_MASS / sys->density[j]) * (term_l + term_r);

                float gradient_x = POLY6_CONSTANT * (-6.0f * diff * diff * (sys->x[i] - sys->x[j]));
                float gradient_y = POLY6_CONSTANT * (-6.0f * diff * diff * (sys->y[i] - sys->y[j]));

                sys->ax[i] += P_ij * gradient_x;
                sys->ay[i] += P_ij * gradient_y;
            }

            // acceleration due to viscosity
            {
                float term_l_x = sys->vx[i] / density_i_squared; 
                float term_r_x = sys->vx[j] / density_j_squared;

                float V_ij_x = -VISCOSITY * (PARTICLE_MASS / sys->density[j]) * (term_l_x + term_r_x);

                float term_l_y = sys->vy[i] / density_i_squared;
                float term_r_y = sys->vy[j] / density_j_squared;

                float V_ij_y = -VISCOSITY * (PARTICLE_MASS / sys->density[j]) * (term_l_y + term_r_y);

                float laplacian = POLY6_CONSTANT * (24.0f * dist_squared * diff - 12.0f * diff * diff);

                sys->ax[i] += V_ij_x * laplacian;
                sys->ay[i] += V_ij_y * laplacian;
            }
        }

        // acceleration due to external forces
        sys->ax[i] += (1.0f / sys->density[i]) * sys->fx[i];
        sys->ay[i] += (1.0f / sys->density[i]) * sys->fy[i];

        // acceleration due to gravity
        sys->ay[i] += GRAVITY;
    }
 
    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->vx[i] += sys->ax[i] * dt;
        sys->x[i]  += sys->vx[i] * dt;
    }

    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->vy[i] += sys->ay[i] * dt;
        sys->y[i]  += sys->vy[i] * dt;
    }
}


