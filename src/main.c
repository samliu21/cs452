#include "allocator.h"
#include "common.h"
#include "exception.h"
#include "interrupt.h"
#include "k2_perf_test.h"
#include "k2_user_tasks.h"
#include "k3_user_tasks.h"
#include "priority_queue.h"
#include "rpi.h"
#include "syscall_asm.h"
#include "syscall_handler.h"
#include "task.h"
#include "test.h"

extern void setup_mmu();

int kmain()
{
#if defined(MMU)
    setup_mmu();
#endif

    gpio_init();
    uart_config_and_enable(CONSOLE);
    uart_puts(CONSOLE, "\r\nconsole loaded!\r\n");

    // run tests and initialize exception vector
    init_interrupts();
    init_vbar();
    tests();

    // create kernel task
    task_t kernel_task;
    kernel_task.tid = 0;

    // create allocator
    task_t tasks[NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, NUM_TASKS);
    char* stack = USER_STACK_START;
    // create initial task
    uint64_t n_tasks = 1;
#ifdef PERFTEST
    task_t* initial_task = allocator_new_task(&allocator, stack, n_tasks++, 1, &k2_perf_initial_task, &kernel_task);
#else
    task_t* initial_task = allocator_new_task(&allocator, stack, n_tasks++, 1, &k3_initial_user_task, &kernel_task);
#endif
    // create PQ with initial task in it
    priority_queue_t scheduler = pq_new();
    pq_add(&scheduler, initial_task);

    // blocked queues
    queue_t tasks_waiting_for_send = queue_new();
    queue_t tasks_waiting_for_reply = queue_new();
    queue_t tasks_waiting_for_event = queue_new();

    // timer
    uint32_t next_tick = timer_get_us() + 10000;

    // kernel context
    main_context_t context;
    context.kernel_task = &kernel_task;
    context.allocator = &allocator;
    context.stack = stack;
    context.n_tasks = &n_tasks;
    context.scheduler = &scheduler;
    context.tasks_waiting_for_send = &tasks_waiting_for_send;
    context.tasks_waiting_for_reply = &tasks_waiting_for_reply;
    context.tasks_waiting_for_event = &tasks_waiting_for_event;
    context.next_tick = next_tick;

    while (!pq_empty(&scheduler)) {
        context.active_task = pq_pop(&scheduler);
        ASSERT(context.active_task->state == READY, "active task is not in ready state");

        uint64_t esr = enter_task(&kernel_task, context.active_task);

        uint64_t syndrome = esr & 0xFFFF;
        uart_printf(CONSOLE, "syndrome: %u\r\n", syndrome);

        switch (syndrome) {
        case SYSCALL_CREATE: {
            create_handler(&context);
            break;
        }
        case SYSCALL_MYTID: {
            my_tid_handler(&context);
            break;
        }
        case SYSCALL_MYPARENTTID: {
            my_parent_tid_handler(&context);
            break;
        }
        case SYSCALL_YIELD: {
            break;
        }
        case SYSCALL_EXIT: {
            exit_handler(&context);
            break;
        }
        case SYSCALL_SEND: {
            send_handler(&context);
            break;
        }
        case SYSCALL_RECEIVE: {
            receive_handler(&context);
            break;
        }
        case SYSCALL_REPLY: {
            reply_handler(&context);
            break;
        }
        case SYSCALL_AWAIT_EVENT: {
            await_event_handler(&context);
            break;
        }
        case INTERRUPT_CODE: {
            uint64_t iar = *(volatile uint32_t*)GICC_IAR;
            uint64_t interrupt_id = iar & INTERRUPT_ID_MASK;
            switch (interrupt_id) {
            
            case INTERRUPT_ID_TIMER: {
                while (!queue_empty(&tasks_waiting_for_event)) {
                    task_t* task = queue_pop(&tasks_waiting_for_event);
                    pq_add(&scheduler, task);
                    task->state = READY;
                }

                uart_printf(CONSOLE, "Clock interrupt\r\n");
                *(BASE_SYSTEM_TIMER + CS_OFFSET) &= ~2;
                *(BASE_SYSTEM_TIMER + C1_OFFSET) = 0;
                break;
            }
            default: {
                ASSERT(0, "unrecognized interrupt\r\n");
                for (;;) { }
            }
            }

            stop_interrupt(interrupt_id);
            break;
        }
        default: {
            ASSERT(0, "unrecognized syscall\r\n");
            for (;;) { }
        }
        }

        if (syndrome != SYSCALL_EXIT && context.active_task->state == READY) {
            pq_add(&scheduler, context.active_task);
        }
    }

    uart_puts(CONSOLE, "No tasks to run\r\n");
    for (;;) { }
}
