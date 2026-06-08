#include "arena.h"
#include <stdlib.h>

#define ARENA_SIZE 64 * 1024 * 1024

void arena_init(Arena *arena) {
    if (arena->mem_arena == NULL)
        arena->mem_arena = (void *) malloc(ARENA_SIZE);

    if (arena->mem_arena == NULL)
        exit(-1);
}

void arena_free(Arena *arena) {
    free(arena->mem_arena);
}

void arena_clear(Arena *arena) {
    arena->offset = 0;
}

void *arena_alloc(Arena *arena, size_t unit_size, size_t num_units) {
    size_t alloc_size = unit_size * num_units;
    alloc_size = (alloc_size + 7) & ~(size_t)7;

    if (arena->offset + alloc_size >= ARENA_SIZE)
        return NULL;

    void *return_ptr = (char *)arena->mem_arena + arena->offset;
    arena->offset += alloc_size;

    return return_ptr;
}
