#include "interrupt.h"
#include "common.h"
#include "rpi.h"
#include "timer.h"
#include "uart_server.h"

void set_route_to_core0(uint64_t interrupt_id)
{
    *(GICD_ITARGETS + interrupt_id) = 1;
}

void set_interrupt_enable(uint64_t interrupt_id)
{
    *(GICD_ISENABLE + (interrupt_id / 8)) = 1 << (interrupt_id % 8);
}

void enable_uart_read_interrupts(int line)
{
    UART_REG(line, UART_IMSC) |= UART_IMSC_RXIM | UART_IMSC_RTIM;
}

void disable_uart_read_interrupts(int line)
{
    UART_REG(line, UART_IMSC) &= ~(UART_IMSC_RXIM | UART_IMSC_RTIM);
    UART_REG(line, UART_ICR) |= UART_ICR_RXIC | UART_ICR_RTIM;
}

void enable_uart_write_interrupts(int line)
{
    UART_REG(line, UART_IMSC) |= UART_IMSC_TXIM;
}

void disable_uart_write_interrupts(int line)
{
    UART_REG(line, UART_IMSC) &= ~UART_IMSC_TXIM;
    UART_REG(line, UART_ICR) |= UART_ICR_TXIC;
}

void enable_uart_cts_interrupts(int line)
{
    UART_REG(line, UART_IMSC) |= UART_IMSC_CTSMIM;
}

void clear_uart_cts_interrupts(int line)
{
    // UART_REG(CONSOLE, UART_IMSC) &= ~UART_IMSC_CTSMIM;
    UART_REG(line, UART_ICR) |= UART_ICR_CTSMIC;
}

void init_interrupts()
{
    set_route_to_core0(INTERRUPT_ID_TIMER);
    set_interrupt_enable(INTERRUPT_ID_TIMER);

    set_route_to_core0(INTERRUPT_ID_UART);
    set_interrupt_enable(INTERRUPT_ID_UART);

    enable_uart_cts_interrupts(MARKLIN);
    // enable_uart_read_interrupts();
    // enable_uart_write_interrupts();
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
        uint32_t terminal_mis = UART_REG(CONSOLE, UART_MIS);
        uint32_t marklin_mis = UART_REG(MARKLIN, UART_MIS);
        ASSERT(!terminal_mis || !marklin_mis, "both terminals have interrupts");

        queue_t* waiting_queue;
        int line;
        uint32_t mis;
        if (terminal_mis) {
            waiting_queue = context->tasks_waiting_for_terminal;
            line = CONSOLE;
            mis = terminal_mis;
        } else {
            waiting_queue = context->tasks_waiting_for_marklin;
            line = MARKLIN;
            mis = marklin_mis;
        }

        if (queue_empty(waiting_queue)) {
            UART_REG(line, UART_ICR) |= UART_ICR_RXIC | UART_ICR_TXIC | UART_ICR_RTIM | UART_ICR_CTSMIC;
            break;
        }
        ASSERT(waiting_queue->size == 1, "more than one task waiting for uart");

        task_t* uart_task = queue_pop(waiting_queue);
        pq_add(context->scheduler, uart_task);
        uart_task->state = READY;

        if ((mis & UART_MIS_RXMIS) || (mis & UART_MIS_RTIM)) {
            disable_uart_read_interrupts(line);
            uart_task->registers[0] = REQUEST_READ_AVAILABLE;
        }
        if (mis & UART_MIS_TXMIS) {
            disable_uart_write_interrupts(line);
            uart_task->registers[0] = REQUEST_WRITE_AVAILABLE;
        }
        if (mis & UART_MIS_CTSMMIS) {
            ASSERT(line == MARKLIN, "CTS from console");
            // if (line == CONSOLE) {
            //     uart_puts(CONSOLE, "CTS from console\r\n");
            // } else {
            //     uart_puts(CONSOLE, "CTS from marklin\r\n");
            // }
            // uint32_t fr = UART_REG(line, UART_FR);
            // uart_putc(CONSOLE, fr & UART_FR_CTS ? '1' : '0');
            // uart_puts(CONSOLE, "\r\n");
            clear_uart_cts_interrupts(line);
            uart_task->registers[0] = REQUEST_CTS_AVAILABLE;
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
