#ifndef CHUNK_H
#define CHUNK_H


#include <stdbool.h>

#include "types.h"


#define MCHUNK_PTR_TO_PTMALLOC2_PTR(ptr) (ptr + 2 * sizeof(size_t))

#define IS_MMAPPED(chunk) (*((size_t*)chunk - 1) & 2)

#define HEAP_INFO(ptr) ((heap_info_proxy*)((long)ptr & ~(HEAP_M_SIZE -1)))

#define ARENA(ptr) (HEAP_INFO(ptr)->arena)

#define MAIN(ptr) (!(*((size_t*)ptr - 1 ) & 4))

#define PREV_INUSE(chunk) (*((size_t*)chunk - 1) & 1)

#define SIZE_FIELD(ptr) (*((size_t*)ptr -1))

#define SIZE(ptr) (SIZE_FIELD(ptr) & ~(7))

#define FLAGS(ptr) (SIZE_FIELD(ptr) & 7)

#define TOP(ar) (SIZE(MCHUNK_PTR_TO_PTMALLOC2_PTR(ar->top)))

void set_chunk_size(ptmalloc2_ptr ptr, size_t size);
void set_chunk_size_head(ptmalloc2_ptr ptr, size_t size);


#endif
