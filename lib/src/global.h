#ifndef GLOBAL_H
#define GLOBAL_H


//GLOBAL VARIABLES
extern size_t TOP_PAD;
extern size_t HEAP_M_SIZE;
extern size_t MMAP_THRESHOLD;
extern size_t MAX_MMAP_THRESHOLD;
extern size_t TRIM_THRESHOLD;


//EXTERNAL FUNCTIONS
extern void* __libc_malloc(size_t size);
extern void __libc_free(void* ptr);
extern void* __libc_calloc(size_t num, size_t size);
extern void* __libc_realloc(void* ptr, size_t size);

#endif
