#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct ArenaPage {
    void   *memory;
    size_t offset;

    struct ArenaPage *next_page;
} ArenaPage;

typedef struct {
    ArenaPage *first_page;
    ArenaPage *last_page;
} Arena;

void arena_init(Arena *arena);
void arena_free(Arena *arena);
void arena_clear(Arena *arena);

void *arena_alloc(Arena *arena, size_t unit_size, size_t num_units);

#endif // !ARENA_H
