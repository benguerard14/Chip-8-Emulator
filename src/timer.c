#include "timer.h"
#include <time.h>
  
time_t start;

void timer_init(){
  start = clock();
}

uint8_t tick_passed(){
  time_t t = clock();
  if(((float)t - (float)start) / CLOCKS_PER_SEC > 0.0167){
    start = clock();
    return 1;
  }
  return 0;
}
