#include <malloc.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "types.h"
#include "chunk.h"
#include "arena.h"
#include "cpu_monitor.h"


//STATIC DATA
 
//Synchronization
bool init = false;
bool init_barrier = false;
bool trim_barrier = false;

//User-controllable sensitivity
float tune = 1;

size_t TOP_PAD = 0;
size_t TRIM_THRESHOLD = 100000;
size_t HEAP_M_SIZE = 8388608 * sizeof(long);
size_t MMAP_THRESHOLD = 128 * 1024;
size_t MAX_MMAP_THRESHOLD = 64 * 1024 * 1024;

//FUNCTIONS
/*bool lock = false;
void debug_log(char* str)
{
	if (__sync_bool_compare_and_swap(&lock, false, true))
	{
		printf(str);
		lock = false;
	}
}
*/



//Initialization. Executed on first malloc call.
static ptmalloc2_ptr allocate(size_t size, int num){
  char buf[256];
  ptmalloc2_ptr ptr;
  if (!init && __sync_bool_compare_and_swap(&init_barrier,false,true)){	 
    //printf("In initialize\n");
    mallopt(M_TRIM_THRESHOLD, -1);
    init_cpu_monitor();    
    ptr = num >= 0 ? __libc_calloc(num, size) : __libc_malloc(size);
    
    init_arenas(ptr);
    init = true;
    //printf("Na init arenas, ptr = %p\n",ptr);
  }
  else {    
    ptr = num >= 0 ? __libc_calloc(num, size) : __libc_malloc(size);
  }
  //snprintf(buf,256,"Ptr = %p, size = %lu\n", ptr, size);
  //debug_log(buf);
  if (!IS_MMAPPED(ptr) && init)
  {
    size_t size_malloced = SIZE(ptr);
    //If non-main arena, update meta data
    if (!MAIN(ptr))
    {
      arena* ar = ARENA(ptr);
      if (!arena_exists(ar)) add_arena(ar);
      else {
        heap_info_proxy* info = HEAP_INFO(ptr);  
        arena* ar = info->arena;
        //ptmalloc2_ptr top = MCHUNK_PTR_TO_PTMALLOC2_PTR(ar->top);
        //size_t top_size = SIZE(top);
          //printf("Expanding top in malloc\n");
          size_t top_mprotect = info->mprotect_size - info->size;
          if (top_mprotect < 0.25 * TOP_PAD) expand_arena(ar);
          }
    }
  }
  return ptr;
}



static inline void update_thresholds()
{
//if (arena_exists(main_arena)) printf("Main arena incorrect!\n");	


	//printf("In need trim\n");
  //Get current memory state
  mem_state state = get_mem_state();
  size_t used_size = state.used;
  size_t top_size = state.top;
  
  size_t base;
  //If allocated memory is smaller than 500kB, use fixed base threshold of 100kB
  if (used_size < 500000)  base = 100000;
  //If memory is smaller than 1MB, use base threshold of half the allocated memory
  else if (used_size < 1000000) base = 0.5*used_size;
  //If smaller than 1GB, linearly decrease the percentage of memory that the threshold value represents. 
  else if (used_size < 1000000000) base = 0.1 * used_size + 400000;
  //If more than 1GB allocated, use fixed threshold of 100MB.
  else base = 100000000;
 
  //More CPUs means TLB shootdowns are more expensive, so increase threshold based on number of CPUs used.
  //Allow tuning by user
  size_t new_trim_threshold = base * (1 + ((float)used_cpus) / 100.0) * tune;

  if (new_trim_threshold > 1.25 * TRIM_THRESHOLD 
      || new_trim_threshold < 0.75 * TRIM_THRESHOLD)
  {	  
    TRIM_THRESHOLD = new_trim_threshold;
    int n_arenas = num_arenas();
    size_t new_top_pad = new_trim_threshold / 4 / n_arenas;
    TOP_PAD = new_top_pad;
    mallopt(M_TOP_PAD, new_top_pad);
  }
}






//Malloc wrapper
void* malloc(size_t size){  
	//return __libc_malloc(size);
    //debug_log("In malloc\n");
    return allocate(size, -1);
    //debug_log("Aan einde malloc\n");
    //process_allocation(mem);
    //char buf[256];
    //snprintf(buf,256,"In malloc, ptr = %p\n", mem);
    //debug_log(buf);
}

//Free wrapper
void free(void* ptr){
    //Many lib4c functions can call free(NULL), so check is necessary
    //
    //char buf[256];
    //snprintf(buf,256,"In free, ptr = %p\n", ptr);
    //debug_log(buf);
    //debug_log("In free\n");
    
    
    if (ptr != NULL && init)
    {
	bool main = MAIN(ptr);
	bool mmapped = IS_MMAPPED(ptr);
	size_t size = SIZE(ptr);

	if (mmapped 
	    && size > MMAP_THRESHOLD 
	    && size <= MAX_MMAP_THRESHOLD)
	{
	  MMAP_THRESHOLD = 1.1 * size > MAX_MMAP_THRESHOLD ? MAX_MMAP_THRESHOLD : 1.1 * size;	 
	  //printf("adjusting MMAP_THRESHOLD\n");
	  mallopt(M_MMAP_THRESHOLD, MMAP_THRESHOLD);
	}

	arena* ar; 
	size_t old_top_size;	
	if (init && !mmapped) {
	  if (main) ar = main_arena;
	  else {
            ar = ARENA(ptr);	  
	    if (!arena_exists(ar)) add_arena(ar);
	  }
	  old_top_size = TOP(ar);
	}

        __libc_free(ptr);


	size_t new_top_size;
	if (init && !mmapped)
	{
	  new_top_size = TOP(ar); 

	  if (new_top_size > old_top_size 
	      && new_top_size > 4 * TOP_PAD
	      && !trim_barrier
	      && __sync_bool_compare_and_swap(&trim_barrier, false, true))
	  {            	
	    //trim_barrier = true;
	    if (need_trim()){
	      //printf("Trimming arenas\n");
	      trim_arenas();	    
	      //printf("Updating threshodls\n");
	      update_thresholds();
	    }
	    trim_barrier = false;
          }
	}	
    }
    else __libc_free(ptr);
}

void* calloc(size_t num, size_t size){
    //debug_log("In calloc\n");
	return allocate(size, num);
	
	//debug_log("Out calloc\n");
	//process_allocation(mem);
    //char buf[256];
    //snprintf(buf,256,"In calloc, ptr = %p\n", mem);
    //debug_log(buf);
}

void* realloc(void* ptr, size_t size){
    //debug_log("In realloc\n");
    ptmalloc2_ptr mem = __libc_realloc(ptr,size);
    //process_allocation(mem);
    return mem;
}

//Allow user to control the trade-off between memory efficiency and TLB shootdowns
//Higher values decrease shootdowns and memory efficiency, lower values increase both
//Default value is 1
//Returns 0 on success, -1 when input is invalid
int set_sensitivity(float val){
    if (val > 0) {
      tune = val;
      return 0;
    }
    else return -1;
}
