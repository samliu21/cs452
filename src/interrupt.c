#include "interrupt.h"
#include "common.h"
#include "rpi.h"

void interrupt_handler()
{
    uint64_t iar = *(volatile uint32_t*)GICC_IAR;
    uint64_t interrupt_id = iar & INTERRUPT_ID_MASK;
    uart_printf(CONSOLE, "Interrupt ID: %u\n", interrupt_id);
    for (;;) { }
}

void init_interrupts()
{
    *(volatile uint32_t*)GICD_BASE = 3;
    *(volatile uint32_t*)GICC_BASE = 3;
    uint32_t rg = *(volatile uint32_t*)GICD_BASE;
    uart_printf(CONSOLE, "GICD_BASE: %u\n", rg);

    uart_puts(CONSOLE, "initializing interrupts\n");

    // route interrupt to cpu0
    *(volatile uint32_t*)(GICD_ITARGETS + 96) |= 1 << 8;

    // enable interrupt 97
    // 97/8 = 12R1
    *(volatile uint32_t*)(GICD_ISENABLE + 12) |= 2;
}
