#include "timer.h"

uint32_t timer_get_us()
{
    return *(volatile uint32_t*)(BASE_SYSTEM_TIMER + CLO_OFFSET);
}

uint32_t timer_get_ms()
{
    return timer_get_us() / 1000;
}

void timer_set_delay(uint64_t ms) {
	uint64_t end_time = timer_get_us() + 1000 * ms;
	*(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET) = end_time;
}
