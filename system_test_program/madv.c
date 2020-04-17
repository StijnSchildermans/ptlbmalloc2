#include <pthread.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <math.h>

int THREAD_COUNT;

void* madv(void* mem){
  for (long i=0;i<1000;i++) madvise(*((char**) mem),4096,MADV_DONTNEED);
}

int main(int argc, char** argv){
  void *mem;
  posix_memalign(&mem, 4096, 8192);
  pthread_t threads[16];

  for (int i = 0; i<16;i++) pthread_create(&threads[i], NULL, madv, &mem);
  for (int i = 0; i<16;i++) pthread_join(threads[i], NULL);

  return 0;
}




