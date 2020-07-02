#ifndef ARENA_H
#define ARENA_H


#include <stddef.h>
#include <stdbool.h>

#include "types.h"


extern arena* main_arena;


void init_arenas(ptmalloc2_ptr ptr);
void add_arena(arena* ar);

bool arena_exists(arena* ar);
int num_arenas();

mem_state get_mem_state();
void trim_arenas();
void expand_arena(arena* ar);
bool need_trim();

#endif
