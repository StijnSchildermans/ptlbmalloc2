#ifndef PTLBMALLOC2_H
#define PTLBMALLOC2_H


extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* calloc(size_t num, size_t size);
extern void* realloc(void* ptr, size_t size);


int set_sensitivity(float val);


#endif
