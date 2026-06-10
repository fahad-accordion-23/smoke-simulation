#include "arena.h"
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4 * 1024 * 1024

ArenaPage *get_new_page() {
    void *mem = malloc(sizeof(ArenaPage) + PAGE_SIZE);

    if (mem == NULL)
        return NULL;

    ArenaPage *new_page = (ArenaPage*) mem;
    new_page->memory    = (void*) ((char*) mem + sizeof(ArenaPage));
    new_page->offset    = 0;
    new_page->next_page = NULL;

    return new_page;
}

void arena_init(Arena *arena) {
    arena->first_page = get_new_page();

    if (arena->first_page == NULL)
        exit(-1);

    arena->last_page = arena->first_page;
}

void arena_free(Arena *arena) {
    ArenaPage *current_page = arena->first_page;

    while (current_page != NULL) {
        ArenaPage* next_page = current_page->next_page;
        free((void*) current_page);
        current_page = next_page;
    }
}

void arena_clear(Arena *arena) {
    ArenaPage *current_page = arena->first_page;

    while (current_page != NULL) {
        current_page->offset = 0;
        memset(current_page->memory, 0, PAGE_SIZE);
        current_page = current_page->next_page;
    }
}

void *arena_alloc(Arena *arena, size_t unit_size, size_t num_units) {
    size_t alloc_size = unit_size * num_units;
    alloc_size = (alloc_size + 7) & ~(size_t)7;

    if (alloc_size > PAGE_SIZE)
        return NULL; // TODO: handle case

    void      *return_ptr;
    ArenaPage *current_page = arena->first_page;

    while (current_page != NULL) {
        if (PAGE_SIZE - current_page->offset >= alloc_size) {
            return_ptr = (void*) ((char*) current_page->memory + current_page->offset);
            current_page->offset += alloc_size;

            goto success;
        } else {
            current_page = current_page->next_page;
            continue;
        }
    }

    /* no page with space available found */
    ArenaPage *new_page = get_new_page();

    arena->last_page->next_page = new_page;
    arena->last_page = new_page;

    return_ptr = (void*) ((char*) arena->last_page->memory + arena->last_page->offset);
    arena->last_page->offset += alloc_size;

success:
    return return_ptr;
}
