#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <stddef.h>
#include "arena.h"

typedef struct {
    float *x;
    float *y;
    float *vx;
    float *vy;
    size_t max_particles;

    int bound_x;
    int bound_y;
} ParticleSystem;

void PS_init(ParticleSystem *sys, Arena *arena, size_t num_particles, int bound_x, int bound_y);
void PS_generate_random_particles(ParticleSystem *sys);
void PS_tick(ParticleSystem *sys, float dt);

#endif // !PARTICLE_SYSTEM_H
