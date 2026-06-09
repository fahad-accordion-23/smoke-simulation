#include <SDL3/SDL.h>
#include <stdlib.h>
#include <unistd.h>
#include "arena.h"
#include "particle_system.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_PARTICLES 65536

void update_pixel_buffer(ParticleSystem* sys, uint32_t *pixels);

int main(void) {
    Arena arena = { 0 };
    arena_init(&arena);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize! Exitting.\n");
        exit(-1);
    }

    ParticleSystem particle_sys = { 0 };
    PS_init(&particle_sys, &arena, MAX_PARTICLES, SCREEN_WIDTH, SCREEN_HEIGHT);

    // TODO: wrap creation in if stmt
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer("Smoke Simulation", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

    uint32_t *pixels = (uint32_t*) arena_alloc(&arena, sizeof(uint32_t), SCREEN_WIDTH * SCREEN_HEIGHT);
    SDL_Texture *screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    Uint64 last_time = SDL_GetPerformanceCounter();
    SDL_Event event;
    
    while (1) {
        Uint64 current_time = SDL_GetPerformanceCounter();
        float dt = (float) (current_time - last_time) / (float) SDL_GetPerformanceFrequency();
        last_time = current_time;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                goto quit;
        }

        PS_generate_random_particles(&particle_sys, dt);
        PS_tick(&particle_sys, dt);

        memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
        update_pixel_buffer(&particle_sys, pixels);

        SDL_UpdateTexture(screen_texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

quit:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    arena_free(&arena);

    return 0;
}

void update_pixel_buffer(ParticleSystem* sys, uint32_t *pixels) {
    for (size_t i = 0; i < sys->alive_particles; i++) {
        int px = (int)sys->x[i];
        int py = (int)sys->y[i];

        if (px >= 0 && px < sys->bound_x && py >= 0 && py < sys->bound_y) {
            pixels[py * SCREEN_WIDTH + px] = 0xFFFFFFFF;
        }
    }
}
