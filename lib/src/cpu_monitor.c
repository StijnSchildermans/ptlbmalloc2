#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/times.h>
#include <sys/sysinfo.h>
#include <unistd.h>

unsigned short max_cpus;
float ticks_per_us; 
struct tms last_times;
unsigned short used_cpus;



//Estimate the number of CPUs currently being used by the program.
static void calc_cpus(int sig){
  //When the number of used CPUs can not be determined, assume all system CPUs are used.
  used_cpus = max_cpus;

  int passed_usecs = 1000000; 
  unsigned short cpus_used;

  //Get CPU time passed
  struct tms cur_times;
  times(&cur_times);
  float cpu_time = (cur_times.tms_utime + cur_times.tms_stime - last_times.tms_utime - last_times.tms_stime)/ticks_per_us; 
  cpus_used = cpu_time/passed_usecs;
  if  (cpus_used > max_cpus || cpus_used == 0) return;	   

  //If successful, set new values 
  last_times = cur_times;
  used_cpus = cpus_used;
}

void init_cpu_monitor(){
    //printf("In init\n");
    times(&last_times);
    max_cpus = get_nprocs();
    ticks_per_us = sysconf(_SC_CLK_TCK)/1000000.0;
    used_cpus = max_cpus;

    signal(SIGALRM, calc_cpus);
    struct itimerval timer;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
}

