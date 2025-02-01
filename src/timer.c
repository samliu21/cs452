#include "timer.h"
#include "rpi.h"

uint32_t timer_get_us()
{
    return *(volatile uint32_t*)(BASE_SYSTEM_TIMER + CLO_OFFSET);
}

uint32_t timer_get_ms()
{
    return timer_get_us() / 1000;
}

uint32_t timer_get_ticks()
{
    return timer_get_us() / US_PER_TICK;
}

void timer_notify_at(uint32_t ms)
{
    *(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET) = ms;
}
