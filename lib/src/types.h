#ifndef TYPES_H
#define TYPES_H


//TYPEDEFS

typedef void* ptmalloc2_ptr;
typedef void* mchunk_ptr;
typedef size_t size_field;
typedef char flags_t;

//STRUCTS

//Placeholder for the malloc_state struct that represents arenas in libc
typedef struct _malloc_state_proxy
{
  int lock;
  int flags;
  int have_fastchunks;
  void* fastbins[10];
  void* top;
  void* last_remainder;
  void* bins[254];
  unsigned int binmap[4];
  struct _malloc_state_proxy *next;
  struct _malloc_state_proxy *next_free;
  size_t attached_threads;
  size_t system_mem;
  size_t max_system_mem;
} arena;

typedef struct _mem_state{
  size_t used;
  size_t top;
} mem_state;

//Placeholder for the heap_info struct that represents heaps in libc
typedef struct _heap_info_proxy
{
  arena* arena; 
  struct _heap_info_proxy *prev; 
  size_t size;   
  size_t mprotect_size; 
} heap_info_proxy;

#endif
