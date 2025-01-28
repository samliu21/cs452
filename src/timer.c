#include "timer.h"

const char* BASE_SYSTEM_TIMER = (const char*)0xfe003000;
const int CLO = 0x04;

uint32_t timer_get_us()
{
    return *(volatile uint32_t*)(BASE_SYSTEM_TIMER + CLO);
}

uint32_t timer_get_ms()
{
    return timer_get_us() / 1000;
}
