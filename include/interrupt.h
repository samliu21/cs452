#ifndef _interrupt_h_
#define _interrupt_h_

#include "common.h"
#include "syscall_handler.h"

#define GIC_BASE (char*)0xff840000
#define GICD_BASE (char*)(GIC_BASE + 0x1000)
#define GICC_BASE (char*)(GIC_BASE + 0x2000)

#define GICC_EOIR (char*)(GICC_BASE + 0x10)
#define GICC_IAR (char*)(GICC_BASE + 0xc)
#define GICD_ITARGETS (char*)(GICD_BASE + 0x800)
#define GICD_ISENABLE (char*)(GICD_BASE + 0x100)

#define INTERRUPT_ID_MASK 0x3ff

#define INTERRUPT_ID_TIMER 97
#define INTERRUPT_ID_UART 153

#define EVENT_TICK 0
#define EVENT_UART 1

void init_interrupts();

void stop_interrupt(uint64_t iar);

void handle_interrupt(main_context_t* context);

#endif
