#include <stdio.h>
#include <stdbool.h>

#include "global.h"
#include "types.h"


void set_chunk_size(ptmalloc2_ptr ptr, size_t size){
  *((size_t*)ptr - 1) = size;
  *((size_t*)(ptr + size - 2)) = size;
}

void set_chunk_size_head(ptmalloc2_ptr ptr, size_t size){
  *((size_t*)ptr - 1) = size; 
}
