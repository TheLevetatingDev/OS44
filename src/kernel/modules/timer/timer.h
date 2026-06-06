#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency);
void timer_handler(void);
uint64_t timer_get_uptime_seconds(void);
uint64_t timer_get_ticks(void);

#endif
