#include "particle_system.h"
#include <stdlib.h>

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
        sys->y[i] = (float) sys->bound_y - 50.0f;

        sys->vx[i] = ((float) rand() / (float) RAND_MAX) * 30.0f - 15.0f;
        sys->vy[i] = -((float) rand() / (float) RAND_MAX) * 60.0f - 20.0f;
    }
}

void PS_tick(ParticleSystem *sys, float dt) {
    for (size_t i = 0; i < sys->max_particles; i++) {
        sys->x[i] += sys->vx[i] * dt;
        sys->y[i] += sys->vy[i] * dt;
    }
}
