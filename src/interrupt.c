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
    uint32_t iar = *(volatile uint32_t*)GICC_IAR;

    // stop interrupt
    *(volatile uint32_t*)GICC_EOIR = iar;

    uint64_t interrupt_id = iar & INTERRUPT_ID_MASK;
    switch (interrupt_id) {
    case INTERRUPT_ID_TIMER: {
        while (!queue_task_empty(context->tasks_waiting_for_timer)) {
            task_t* task = queue_task_pop(context->tasks_waiting_for_timer);
            pq_task_add(context->scheduler, task);
            task->state = READY;
        }

        *(volatile uint32_t*)(BASE_SYSTEM_TIMER + CS_OFFSET) |= 2;
        *(volatile uint32_t*)(BASE_SYSTEM_TIMER + C1_OFFSET) = 0;
        break;
    }
    case INTERRUPT_ID_UART: {
        uint32_t terminal_mis = UART_REG(CONSOLE, UART_MIS);
        uint32_t marklin_mis = UART_REG(MARKLIN, UART_MIS);

        queue_task_t* waiting_queue;
        int line;
        uint32_t mis;
        for (int i = 0; i < 2; ++i) {
            if (i == 0) {
                // console interrupt
                waiting_queue = context->tasks_waiting_for_terminal;
                line = CONSOLE;
                mis = terminal_mis;
            } else {
                // terminal interrupt
                waiting_queue = context->tasks_waiting_for_marklin;
                line = MARKLIN;
                mis = marklin_mis;
            }
            if (!mis) {
                continue;
            }
            if (waiting_queue->size == 0) {
                ASSERT(mis & UART_MIS_CTSMMIS, "empty waiting queue should only be possible for CTS");
                clear_uart_cts_interrupts(line);
            }
            if (waiting_queue->size >= 2) {
                uart_puts(CONSOLE, line == CONSOLE ? "console has more than two tasks waiting\r\n" : "marklin has more than two tasks waiting\r\n");
                task_t* t = waiting_queue->head;
                while (t) {
                    uart_printf(CONSOLE, "task %d\r\n", t->tid);
                    t = t->next_task;
                }
            }
            ASSERT(waiting_queue->size == 1, "exactly one task should be waiting for uart");

            // reschedule the task
            task_t* uart_task = queue_task_pop(waiting_queue);
            pq_task_add(context->scheduler, uart_task);
            uart_task->state = READY;

            // handle the interrupt
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
                clear_uart_cts_interrupts(line);
                uart_task->registers[0] = REQUEST_CTS_AVAILABLE;
            }
        }

        break;
    }
    default: {
        ASSERTF(0, "unrecognized interrupt: %u\r\n", interrupt_id);
        for (;;) { }
    }
    }
}
