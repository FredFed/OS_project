#include "util.h"

unsigned long time_difference_ns(stimespec* a, stimespec* b) {
  unsigned long sec_a = a->tv_sec;
  unsigned long sec_b = b->tv_sec;
  unsigned long ns_a = a->tv_nsec;
  unsigned long ns_b = b->tv_nsec;

  unsigned long s_tot = sec_a - sec_b;    // Calculating seconds difference
  unsigned long ns_tot = ns_a - ns_b;     // Calculating nanoseconds difference
  // printf("return value: %d\n", (S_TO_NS(s_tot) + (ns_tot)));   // TODO remove
  return (S_TO_NS(s_tot) + (ns_tot));     // Returns the time difference
}

short compare_time(stimespec* a, stimespec* b) {
  unsigned long sec_a = a->tv_sec;
  unsigned long sec_b = b->tv_sec;
  unsigned long ns_a = a->tv_nsec;
  unsigned long ns_b = b->tv_nsec;

  if(sec_a>sec_b) return -1;
  else if(sec_a<sec_b) return 1;
  else {
    if(ns_a>ns_b) return -1;
    else if(ns_a<ns_b) return 1;
    else return 0;
  }
}

short compare_current_time(const stimespec* b) {
  short errtemp;
  short result;
  stimespec* current=(stimespec*)malloc(sizeof(stimespec));   // TODO mem err

  if((errtemp=clock_gettime(CLOCK_REALTIME, current))==ERR) {     // Setting a time breakpoint for future calculation of the thread's execution time
    perror("Getting clock time: ");     // Printing debug message
    free(current);
    return MEMERR;
  }


  unsigned long sec_a = current->tv_sec;
  unsigned long sec_b = b->tv_sec;
  unsigned long ns_a = current->tv_nsec;
  unsigned long ns_b = b->tv_nsec;

  if(sec_a>sec_b) result=-1;
  else if(sec_a<sec_b) result=1;
  else {
    if(ns_a>ns_b) result=-1;
    else if(ns_a<ns_b) result=1;
    else result=0;
  }

  free(current);
  return result;
}