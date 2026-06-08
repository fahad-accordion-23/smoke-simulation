#include <SDL3/SDL.h>
#include <stdlib.h>
#include "arena.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct {
    float *x;
    float *y;
    float *vx;
    float *vy;
    size_t max_particles;
} ParticleSystem;

void init_particle_system(ParticleSystem *sys, Arena *arena, size_t num_particles);

int main(void) {
    Arena arena = { 0 };
    arena_init(&arena);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize! Exitting.\n");
        exit(-1);
    }

    ParticleSystem particle_system = { 0 };
    init_particle_system(&particle_system, &arena, 5000);

    for (size_t i = 0; i < particle_system.max_particles; i++) {
        particle_system.x[i] = SCREEN_WIDTH / 2.0f;
        particle_system.y[i] = SCREEN_HEIGHT - 50.0f;

        particle_system.vx[i] = ((float) rand() / (float) RAND_MAX) * 30.0f - 15.0f;
        particle_system.vy[i] = -((float) rand() / (float) RAND_MAX) * 60.0f - 20.0f;
    }

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

        memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));

        for (size_t i = 0; i < particle_system.max_particles; i++) {
            particle_system.x[i] += particle_system.vx[i] * dt;
            particle_system.y[i] += particle_system.vy[i] * dt;

            int px = (int)particle_system.x[i];
            int py = (int)particle_system.y[i];

            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                pixels[py * SCREEN_WIDTH + px] = 0xFFFFFFFF;
            }
        }

        SDL_UpdateTexture(screen_texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

quit:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void init_particle_system(ParticleSystem *sys, Arena *arena, size_t num_particles) {
    sys->max_particles = num_particles;
    sys->x = arena_alloc(arena, sizeof(float), num_particles);
    sys->y = arena_alloc(arena, sizeof(float), num_particles);
    sys->vx = arena_alloc(arena, sizeof(float), num_particles);
    sys->vy = arena_alloc(arena, sizeof(float), num_particles);
}
