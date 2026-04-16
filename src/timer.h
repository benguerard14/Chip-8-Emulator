#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define FREQUENCY 60;

void timer_init();
uint8_t tick_passed();

#endif
