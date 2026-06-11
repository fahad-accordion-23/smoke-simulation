#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include "arena.h"

typedef struct {
    int x;
    int y;
} Vector2i;

typedef struct {
    float x;
    float y;
} Vector2f;

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
    size_t start_index;
    size_t count;
} Bin;

typedef struct {

    // properties of the system
    size_t max_particles;

    Vector2i dimensions;

    Vector2i cell_dimensions;
    size_t   cell_rows;
    size_t   cell_cols;
    Bin      *bin;

    /*
     * Particle Data
     *
     * The _s suffix means _sorted. The system uses counting sort to sort particles. After
     * sorting property into property_s, the system just swaps the property and property_s 
     * arrays.
     *
     */

    // float *mass;
    // float *mass_s;

    float *density;

    float *pressure;

    Vector2f *force;
    Vector2f *force_s;

    Vector2f *acc;

    Vector2f *vel;
    Vector2f *vel_s;

    Vector2f *pos;
    Vector2f *pos_s;

    Color *color;
    Color *color_s;

} ParticleSystem;

void PS_init(ParticleSystem *sys,
             Arena *arena,
             size_t num_particles,
             Vector2i dimensions,
             Vector2i cell_dimensions);
void PS_generate_random_particles(ParticleSystem *sys);
void PS_tick(ParticleSystem *sys, float dt);

#endif // !PARTICLE_SYSTEM_H
