#ifndef _timer_h_
#define _timer_h_

#include "common.h"

#define BASE_SYSTEM_TIMER (const char*)0xfe003000
#define CLO_OFFSET 0x04
#define C1_OFFSET 0x10

#define EVENT_TICK 0

uint32_t timer_get_us();
uint32_t timer_get_ms();
void timer_notify_at(uint32_t ms);

#endif
