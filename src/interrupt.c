#include "interrupt.h"
#include "common.h"
#include "rpi.h"

void init_interrupts()
{
    // route interrupt to cpu0
    *(GICD_ITARGETS + 97) = 1;

    // enable interrupt 97
    // 97/8 = 12R1
    *(GICD_ISENABLE + 12) = 2;
}

void stop_interrupt(uint64_t iar)
{

    *(volatile uint32_t*)GICC_EOIR = iar;
}
