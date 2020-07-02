#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h> 
#include <linux/futex.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <pthread.h>

#include "cpu_monitor.h"
#include "chunk.h"
#include "global.h"


int max_arenas;
arena** arenas = NULL;
arena* main_arena;

static inline void set_main_arena(ptmalloc2_ptr ptr)
{
     arena* a = HEAP_INFO(ptr)->arena;
     arena* ar = a->next;
     arena* max = a;
     while (ar != a)
     {
	if ( ar > max) max = ar;
	ar = ar->next;
     }
     main_arena = max;
}

static void* find_main_arena(void* arg)
{
     ptmalloc2_ptr ptr = __libc_malloc(1024);
     set_main_arena(ptr);
     __libc_free(ptr);     
}

void init_arenas(ptmalloc2_ptr ptr)
{
//	printf("In init arenas\n");
     max_arenas = 8 * max_cpus;
     arenas = mmap(NULL, max_arenas * sizeof(arena*), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

     if (MAIN(ptr))
     {
       pthread_t thread;
	     //printf("Is main\n");
       pthread_create(&thread, NULL, &find_main_arena, NULL);
       //while (_main_arena == NULL) usleep(1000);
       pthread_join(thread, NULL);
       //sleep(10);
       //pthread_create(&thread, NULL, &find_main_arena, NULL);
       //printf("Return value of join: %d\n", ret);
       //while (_main_arena == NULL) usleep(1000); 
     }
     else set_main_arena(ptr);
     //printf("Found main arena: %p\n", _main_arena);
}

//Futex syscall wrapper
static inline int futex(int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3)
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr, val3);
}

//Lock an arena
static void lock_arena(arena* ar)
{
    int* lock = &ar->lock;
    if (__sync_val_compare_and_swap(lock,0,1)){
      do {
        int old_val = __sync_val_compare_and_swap(lock,1,2);
        if (old_val != 0) futex(lock, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0); 
      } while (__sync_val_compare_and_swap(lock,0,2) != 0); 
    }   
        
}

//Unlock an arena
static void unlock_arena(arena* ar)
{
  int* lock = &ar->lock;
  int old_val = __sync_lock_test_and_set(lock,0);
  if (old_val > 1) futex(lock, FUTEX_WAKE_PRIVATE, 1, NULL,NULL,0);
}

bool arena_exists(arena* ar)
{
    int i = 0;
    while (i < max_arenas && arenas[i] != NULL){
      if (arenas[i] == ar) return true;
      i++;
    }
    return false;
}

//Add new non-main arena
void add_arena(arena* ar)
{
    int i = 0;
    while (arenas[i] != NULL)
    {
	    if (arenas[i] == ar) return;
	    i++;
    }
    while (__sync_val_compare_and_swap(arenas + i, NULL, ar) != NULL)
    {
	    if (arenas[i] == ar) return;
	    i++; 
    }
}

//Get the amount of used and top memory
mem_state get_mem_state()
{
  mem_state state;    
  lock_arena(main_arena);
  state.top = TOP(main_arena);
  state.used = main_arena->system_mem;  
  unlock_arena(main_arena);
  int i = 0;
  while (i < max_arenas && arenas[i] != NULL)
  {
    arena* ar = arenas[i];
    lock_arena(ar);
    state.used += ar->system_mem;
    state.top += TOP(ar);   
    unlock_arena(ar);
    i++;
  }
  return state;
}

bool need_trim()
{
  size_t top;
	  lock_arena(main_arena);
	  top = TOP(main_arena);
	  unlock_arena(main_arena);
  int i = 0;
  while (i < max_arenas && arenas[i] != NULL)
  {
    arena* ar = arenas[i];
    lock_arena(ar);
    top += TOP(ar);   
    unlock_arena(ar);
    if (top > TRIM_THRESHOLD) return true;
    i++;
  }
  return false;
}

//Trimming function for non-main arenas
static inline void trim_arena(arena* ar)
{
	//printf("Trimming arena\n");
    lock_arena(ar);
    mchunk_ptr top = ar->top;
    ptmalloc2_ptr top_chunk = MCHUNK_PTR_TO_PTMALLOC2_PTR(top);
    size_t top_size = SIZE(top_chunk);
        
    void* addr = (void*)(((long)(top + TOP_PAD) | 4095) + 1);
    unsigned long len = (unsigned long)(top + top_size - addr);

    if (top_size > 2 * TOP_PAD
        && top_size - len > 32){
        //Trim to 128 kB
        size_t size_new_top = (top_size - len);
	//Use madvise to trim arena
        madvise(addr, len, MADV_DONTNEED);
        //printf("Trimmed non-main arena, new top size  = %lu\n", size_new_top);
	//Update meta data
        set_chunk_size_head(top_chunk, size_new_top | 1);

        heap_info_proxy* top_heap_info = HEAP_INFO((top));
        top_heap_info->size -= len;
        ar->system_mem -= len;
    }
    unlock_arena(ar);
}

//Trim all arenas
void trim_arenas()
{
  //Trim main arena to 128 kB
  malloc_trim(TOP_PAD);	
  //main_arena.top = TOP_PAD;
  //printf("Trimmed main arena\n");


  //Trim non-main arenas
  int i =0;
  while ( i < max_arenas && arenas[i] != NULL ){
    trim_arena(arenas[i]);	    
    i++;
  } 
}

int num_arenas()
{
  int i = 0;
  while (i < max_arenas && arenas[i] != NULL) i++;
  return i + 1;
}


void expand_arena(arena* ar)
{
  lock_arena(ar);
  heap_info_proxy* info = HEAP_INFO(ar->top);
  //Recalculate after locking
  size_t top_mprotect = info->mprotect_size - info->size;
  if (top_mprotect >= 0.25 * TOP_PAD)
  {
    unlock_arena(ar);
    return;
  }

  //printf("Calling mprotect\n");
  void* addr = (void*)info + info->mprotect_size;
  size_t len = ((TOP_PAD - top_mprotect) | 4095) + 1;
  if (info->mprotect_size + len < HEAP_M_SIZE 
      && mprotect(addr, len, PROT_READ | PROT_WRITE) == 0)
  { 
    //printf("Mprotect succesful!\n");
    info->mprotect_size += len;     
    ar->system_mem += len;
    if (ar->system_mem > ar->max_system_mem) ar->max_system_mem = ar->system_mem;
  }    
  else 
  {
    unlock_arena(ar);
    return;
  }
  unlock_arena(ar);
}
