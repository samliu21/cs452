#include "interrupt.h"
#include "common.h"
#include "rpi.h"

// void interrupt_handler()
// {
//     uint64_t iar = *(volatile uint32_t*)GICC_IAR;
//     uint64_t interrupt_id = iar & INTERRUPT_ID_MASK;
//     uart_printf(CONSOLE, "Interrupt ID: %u\n", interrupt_id);
//     ASSERT(interrupt_id == 97, "Unknown Interrupt ID");
// }

void init_interrupts()
{
    // route interrupt to cpu0
    *(GICD_ITARGETS + 97) = 1;

    // enable interrupt 97
    // 97/8 = 12R1
    *(GICD_ISENABLE + 12) = 2;
}

void stop_interrupt(uint64_t interrupt_id)
{
    *(GICC_EOIR) = interrupt_id;
}
