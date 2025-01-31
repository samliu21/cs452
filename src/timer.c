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

void timer_set_delay(uint64_t ms)
{
    uint32_t end_time = timer_get_us() + 1000 * ms;
    uint32_t t;
    t = *(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET);
    uart_printf(CONSOLE, "before: %u\n", t);
    uart_printf(CONSOLE, "end_time: %u\n", timer_get_us());

    *(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET) = end_time;

    t = *(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET);
    uart_printf(CONSOLE, "after: %u\n", t);
}
