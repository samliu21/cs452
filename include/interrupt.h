#ifndef _interrupt_h_
#define _interrupt_h_

#define GIC_BASE (const char*)0xff840000
#define GICD_BASE (const char*)(GIC_BASE + 0x1000)
#define GICC_BASE (const char*)(GIC_BASE + 0x2000)

#define GICC_IAR (const char*)(GICC_BASE + 0x00c)
#define GICD_ITARGETS (const char*)(GICD_BASE + 0x800)
#define GICD_ISENABLE (const char*)(GICD_BASE + 0x100)

#define INTERRUPT_ID_MASK 0x3ff

void interrupt_handler();

void init_interrupts();

#endif
