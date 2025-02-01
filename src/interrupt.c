#include "interrupt.h"
#include "common.h"
#include "rpi.h"
#include "timer.h"

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

void handle_interrupt(main_context_t* context)
{
    uint64_t iar = *(volatile uint32_t*)GICC_IAR;

    uint64_t interrupt_id = iar & INTERRUPT_ID_MASK;
    switch (interrupt_id) {
    case INTERRUPT_ID_TIMER: {
        while (!queue_empty(context->tasks_waiting_for_event)) {
            task_t* task = queue_pop(context->tasks_waiting_for_event);
            pq_add(context->scheduler, task);
            task->state = READY;
        }

        *(volatile uint32_t*)(BASE_SYSTEM_TIMER + CS_OFFSET) |= 2;
        *(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET) = 0;
        break;
    }
    default: {
        ASSERT(0, "unrecognized interrupt\r\n");
        for (;;) { }
    }
    }
    stop_interrupt(iar);
}
