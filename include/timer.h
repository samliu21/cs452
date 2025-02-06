#ifndef _timer_h_
#define _timer_h_

#include "common.h"

#define BASE_SYSTEM_TIMER (char*)0xfe003000
#define CS_OFFSET 0x00
#define CLO_OFFSET 0x04
#define C1_OFFSET 0x10

#define US_PER_TICK 10000

uint32_t timer_get_us();
uint32_t timer_get_ms();
uint32_t timer_get_ticks();
void timer_notify_at(uint32_t ms);

#endif
