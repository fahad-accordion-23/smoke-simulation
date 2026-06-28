#include <SDL3/SDL.h>
#include <stdlib.h>
#include <unistd.h>
#include "arena.h"
#include "particle_system.h"
#include <stdio.h>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 800
#define CELL_WIDTH    10                 // px
#define CELL_HEIGHT   10                 // px
#define MAX_PARTICLES 1000
#define TICK_RATE     200                // Hz
#define PHYSICS_DT    (1.0f / TICK_RATE) // seconds
#define FRAME_RATE    24                 // Hz

#define MAX_TICKS_PER_FRAME (TICK_RATE * 0.25f)

void update_pixel_buffer(ParticleSystem* sys, uint32_t *pixels);

int main(void) {

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize! Exitting.\n");
        exit(-1);
    }
    
    SDL_Window   *window;
    SDL_Renderer *renderer;
    if (!SDL_CreateWindowAndRenderer("Smoke Simulation",
                                     SCREEN_WIDTH,
                                     SCREEN_HEIGHT,
                                     0,
                                     &window,
                                     &renderer)) {
        SDL_Log("Failed to create window and/or renderer! Exiting.\n");
        exit(-1);
    }

    Arena arena = { 0 };
    arena_init(&arena);

    Vector2i dimensions = { SCREEN_WIDTH, SCREEN_HEIGHT };
    Vector2i cell_dimensions = { CELL_WIDTH, CELL_HEIGHT };

    ParticleSystem particle_sys = { 0 };
    PS_init(&particle_sys, &arena, MAX_PARTICLES, dimensions, cell_dimensions);
    PS_generate_boundary_particles(&particle_sys);
    PS_generate_random_particles(&particle_sys);

    uint32_t *pixels;
    pixels = (uint32_t*) arena_alloc(&arena,
                                     sizeof(uint32_t),
                                     SCREEN_WIDTH * SCREEN_HEIGHT);

    SDL_Texture *screen_texture;
    screen_texture = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       SCREEN_WIDTH,
                                       SCREEN_HEIGHT);

    Uint64 last_time = SDL_GetPerformanceCounter();

    float time_since_last_tick   = 0.0f;
    float time_since_last_render = 0.0f;

    SDL_Event event;
    
    while (1) {
        Uint64 current_time = SDL_GetPerformanceCounter();
        float render_dt = (float) (current_time - last_time) / (float) SDL_GetPerformanceFrequency();
        last_time = current_time;

        time_since_last_tick += render_dt;
        time_since_last_render += render_dt;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                goto quit;
        }

        int ticks_processed = 0;
        while (time_since_last_tick >= (1.0f / TICK_RATE) && ticks_processed <= MAX_TICKS_PER_FRAME) {

            time_since_last_tick -= (1.0f / TICK_RATE);
            PS_tick(&particle_sys, PHYSICS_DT);

            ticks_processed += 1;
        }

        if (time_since_last_render >= (1.0f / FRAME_RATE)) {

            time_since_last_render = 0.0f;

            memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
            update_pixel_buffer(&particle_sys, pixels);

            SDL_UpdateTexture(screen_texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, screen_texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }

quit:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    arena_free(&arena);

    return 0;
}

void update_pixel_buffer(ParticleSystem* sys, uint32_t *pixels) {
    for (size_t i = 0; i < sys->num_boundary_particles; i++) {
        int px = (int) sys->pos_b[i].x;
        int py = (int) sys->pos_b[i].y;

        if (px >= 0 && px < sys->dimensions.x && py >= 0 && py < sys->dimensions.y) {
            pixels[py * SCREEN_WIDTH + px] = 0xFFFFFFFF;
        }
    }

    for (size_t i = 0; i < sys->max_particles; i++) {
        int px = (int) sys->pos[i].x;
        int py = (int) sys->pos[i].y;

        if (px >= 0 && px < sys->dimensions.x && py >= 0 && py < sys->dimensions.y) {
            pixels[py * SCREEN_WIDTH + px] = sys->color[i].value;
        }
    }
}
