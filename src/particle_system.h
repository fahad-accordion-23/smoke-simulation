#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include "arena.h"

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t g;
    uint8_t r;
} _Color;

typedef union {
    _Color _color;
    uint32_t value;
} Color;

typedef struct {
    // properties of the system
    size_t max_particles;
    size_t alive_particles;
    int bound_x;
    int bound_y;
    float accumulated_dt;

    // particles data

    // mass not added because assuming uniform mass of all particles
    // float *mass;

    float *density;

    float *pressure;

    float *force_x;
    float *force_y;

    // acceleration
    float *ax;
    float *ay;

    // velocity
    float *vx;
    float *vy;

    // position
    float *x;
    float *y;

    Color *color;
} ParticleSystem;

void PS_init(ParticleSystem *sys, Arena *arena, size_t num_particles, int bound_x, int bound_y);
void PS_generate_random_particles(ParticleSystem *sys, float dt);
void PS_tick(ParticleSystem *sys, float dt);

#endif // !PARTICLE_SYSTEM_H
