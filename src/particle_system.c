#include "particle_system.h"
#include <stdlib.h>
#include <math.h>

#define DRAG_MULTIPLIER 0.05f
#define GRAVITY 9.81f

void PS_init(ParticleSystem *sys, Arena *arena, size_t num_particles, int bound_x, int bound_y) {
    sys->max_particles = num_particles;
    sys->x = arena_alloc(arena, sizeof(float), num_particles);
    sys->y = arena_alloc(arena, sizeof(float), num_particles);
    sys->vx = arena_alloc(arena, sizeof(float), num_particles);
    sys->vy = arena_alloc(arena, sizeof(float), num_particles);
    sys->bound_x = bound_x;
    sys->bound_y = bound_y;
}

void PS_generate_random_particles(ParticleSystem *sys) {
    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->x[i] = (float) sys->bound_x / 2.0f;
    }

    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->y[i] = (float) sys->bound_y - 50.0f;
    }

    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->vx[i] = ((float) rand() / (float) RAND_MAX) * 30.0f - 15.0f;
    }

    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->vy[i] = -((float) rand() / (float) RAND_MAX) * 60.0f - 20.0f;
    }
}

void PS_tick(ParticleSystem *sys, float dt) {
    // y calculations

    for (size_t i = 0; i < sys->max_particles; i++) {
        /* air resistance */
        float drag_y = DRAG_MULTIPLIER * sys->vy[i] * fabsf(sys->vy[i]);
        sys->vy[i] += -drag_y * dt;

        /* gravity */
        sys->vy[i] += GRAVITY * dt;

        /* buoyancy */
        sys->vy[i] -= 15.0f * dt;
    }

    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->y[i] += sys->vy[i] * dt;
    }

    // x calculations

    for (size_t i = 0; i < sys->max_particles; i++) {
        /* air resistance */
        float drag_x = DRAG_MULTIPLIER * sys->vx[i] * fabsf(sys->vx[i]);
        sys->vx[i] += -drag_x * dt;
    }

    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->x[i] += sys->vx[i] * dt;
    }
}
