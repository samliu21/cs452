#include "interrupt.h"
#include "common.h"
#include "rpi.h"
#include "timer.h"

void set_route_to_core0(uint64_t interrupt_id)
{
    *(GICD_ITARGETS + interrupt_id) = 1;
}

void set_interrupt_enable(uint64_t interrupt_id)
{
    *(GICD_ISENABLE + (interrupt_id / 8)) = 1 << (interrupt_id % 8);
}

void enable_uart_read_interrupts()
{
    UART_REG(CONSOLE, UART_IMSC) |= UART_IMSC_RXIM;
}

void disable_uart_read_interrupts()
{
    UART_REG(CONSOLE, UART_IMSC) &= ~UART_IMSC_RXIM;
    UART_REG(CONSOLE, UART_ICR) |= UART_ICR_RXIC;
}

void enable_uart_write_interrupts()
{
    UART_REG(CONSOLE, UART_IMSC) |= UART_IMSC_TXIM;
}

void disable_uart_write_interrupts()
{
    UART_REG(CONSOLE, UART_IMSC) &= ~UART_IMSC_TXIM;
    UART_REG(CONSOLE, UART_ICR) |= UART_ICR_TXIC;
}

void init_interrupts()
{
    set_route_to_core0(INTERRUPT_ID_TIMER);
    set_interrupt_enable(INTERRUPT_ID_TIMER);

    set_route_to_core0(INTERRUPT_ID_UART);
    set_interrupt_enable(INTERRUPT_ID_UART);

    enable_uart_read_interrupts();
    enable_uart_write_interrupts();
}

void handle_interrupt(main_context_t* context)
{
    uint64_t iar = *(volatile uint32_t*)GICC_IAR;

    uint64_t interrupt_id = iar & INTERRUPT_ID_MASK;
    switch (interrupt_id) {
    case INTERRUPT_ID_TIMER: {
        while (!queue_empty(context->tasks_waiting_for_timer)) {
            task_t* task = queue_pop(context->tasks_waiting_for_timer);
            pq_add(context->scheduler, task);
            task->state = READY;
        }

        *(volatile uint32_t*)(BASE_SYSTEM_TIMER + CS_OFFSET) |= 2;
        *(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET) = 0;
        break;
    }
    case INTERRUPT_ID_UART: {
        while (!queue_empty(context->tasks_waiting_for_uart)) {
            task_t* task = queue_pop(context->tasks_waiting_for_uart);
            pq_add(context->scheduler, task);
            task->state = READY;
        }
        uint32_t uart_status = UART_REG(CONSOLE, UART_MIS);

        uart_printf(CONSOLE, "UART interrupt with status: %u\r\n", uart_status);
        if (uart_status & UART_MIS_RXMIS) {
            disable_uart_read_interrupts();
        }
        if (uart_status & UART_MIS_TXMIS) {
            disable_uart_write_interrupts();
        }

        break;
    }
    default: {
        ASSERT(0, "unrecognized interrupt\r\n");
        for (;;) { }
    }
    }

    // stop interrupt
    *(volatile uint32_t*)GICC_EOIR = iar;
}
