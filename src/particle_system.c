#include "particle_system.h"
#include <stdlib.h>
#include <math.h>

#define MIN_INITIAL_VEL_X -5.0f
#define MAX_INITIAL_VEL_X +5.0f

#define MIN_INITIAL_VEL_Y -5.0f
#define MAX_INITIAL_VEL_Y -30.0f

#define SPAWN_RATE 2048
#define DRAG_MULTIPLIER 0.03f
#define GRAVITY +9.81f
#define BUOYANCY -15.0f

float f_generate_rand(float min, float max) {
    return ((float) rand() / (float) RAND_MAX) * (max - min) + min;
}

void PS_init(ParticleSystem *sys, Arena *arena, size_t num_particles, int bound_x, int bound_y) {
    sys->max_particles = num_particles;
    sys->alive_particles = 0;
    sys->bound_x = bound_x;
    sys->bound_y = bound_y;
    sys->accumulated_dt = 0.0f;

    sys->x = arena_alloc(arena, sizeof(float), num_particles);
    sys->y = arena_alloc(arena, sizeof(float), num_particles);
    sys->vx = arena_alloc(arena, sizeof(float), num_particles);
    sys->vy = arena_alloc(arena, sizeof(float), num_particles);
    sys->color = arena_alloc(arena, sizeof(Color), num_particles);
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
        sys->x[i] = (float) sys->bound_x / 2.0f;
    }

    for (size_t i = start_idx; i < final_idx; i++) {
        sys->y[i] = (float) sys->bound_y - 50.0f;
    }

    for (size_t i = start_idx; i < final_idx; i++) {
        sys->vx[i] = f_generate_rand(MIN_INITIAL_VEL_X, MAX_INITIAL_VEL_X);
    }

    for (size_t i = start_idx; i < final_idx; i++) {
        sys->vy[i] = f_generate_rand(MIN_INITIAL_VEL_Y, MAX_INITIAL_VEL_Y);
    }

    for (size_t i = start_idx; i< final_idx; i++) {
        sys->color[i].value = 0xFFFFFFFF;
    }

    sys->alive_particles += batch_size;
}

void PS_tick(ParticleSystem *sys, float dt) {
    /* y calculations */

    for (size_t i = 0; i < sys->alive_particles; i++) {
        // air resistance
        float drag_y = DRAG_MULTIPLIER * sys->vy[i] * fabsf(sys->vy[i]);
        sys->vy[i] += -drag_y * dt;

        // gravity
        sys->vy[i] += GRAVITY * dt;

        // buoyancy
        sys->vy[i] += BUOYANCY * dt;
    }

    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->y[i] += sys->vy[i] * dt;
    }

    /* x calculations */

    for (size_t i = 0; i < sys->alive_particles; i++) {
        /* air resistance */
        float drag_x = DRAG_MULTIPLIER * sys->vx[i] * fabsf(sys->vx[i]);
        sys->vx[i] += -drag_x * dt;
    }

    for (size_t i = 0; i < sys->alive_particles; i++) {
        sys->x[i] += sys->vx[i] * dt;
    }

    // color calculations
    sys->accumulated_dt += dt;
    if (sys->accumulated_dt > 0.1f)
    {
        sys->accumulated_dt = 0.0f;

        for (size_t i = 0; i < sys->alive_particles; i++) {
            if (sys->color[i]._color.a > 0)
                sys->color[i]._color.a -= 1;
        }
    }
    

    // respawn check
    for (size_t i = 0; i < sys->alive_particles; i++) {
        float y = sys->y[i];
        float x = sys->x[i];

        if (y < 0.0f || y >= sys->bound_y || x < 0.0f || x >= sys->bound_x) {
            sys->x[i] = (float) sys->bound_x / 2.0f;
            sys->y[i] = (float) sys->bound_y - 50.0f;
            sys->vx[i] = f_generate_rand(MIN_INITIAL_VEL_X, MAX_INITIAL_VEL_X);
            sys->vy[i] = f_generate_rand(MIN_INITIAL_VEL_Y, MAX_INITIAL_VEL_Y);
            sys->color[i].value = 0xFFFFFFFF;
        }
    }
}


