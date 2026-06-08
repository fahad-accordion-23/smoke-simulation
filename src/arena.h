#include <stddef.h>

typedef struct {
    void *mem_arena;
    size_t offset;

} Arena;

void arena_init(Arena *arena);
void arena_free(Arena *arena);
void arena_clear(Arena *arena);

void *arena_alloc(Arena *arena, size_t unit_size, size_t num_units);
