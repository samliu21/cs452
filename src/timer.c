#include "timer.h"

const char* BASE_SYSTEM_TIMER = (const char*)0xfe003000;
const int CLO = 0x04;

uint64_t timer_get()
{
    return *(volatile uint64_t*)(BASE_SYSTEM_TIMER + CLO);
}
