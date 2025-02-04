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

#define PRINT_PERF_AFTER_MS 50

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
    int failed = tests();
    if (failed) {
        for (;;) { }
    }

    // clear screen
    uart_puts(CONSOLE, "\033[2J\r\n\r\n");

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
    uint32_t next_tick = timer_get_us() + US_PER_TICK;

    // performance metrics
    uintmap_t performance_map = uintmap_new();
    uint64_t kernel_time_start = timer_get_us(), kernel_time_duration, kernel_time_end, user_time_duration;

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
    context.performance_map = &performance_map;

    uint64_t last_perf_print = timer_get_ms();
    uint64_t idle_tid = 0;

    while (!pq_empty(&scheduler)) {
        context.active_task = pq_pop(&scheduler);
        ASSERT(context.active_task->state == READY, "active task is not in ready state");

        kernel_time_end = timer_get_us();
        kernel_time_duration = kernel_time_end - kernel_time_start;
        uintmap_increment(&performance_map, kernel_task.tid, kernel_time_duration);

        uint64_t esr = enter_task(&kernel_task, context.active_task);

        kernel_time_start = timer_get_us();
        user_time_duration = kernel_time_start - kernel_time_end;
        uintmap_increment(&performance_map, context.active_task->tid, user_time_duration);

        int should_print_perf = timer_get_ms() - last_perf_print > PRINT_PERF_AFTER_MS;
        if (should_print_perf) {
            // get idle task's tid if we haven't set it yet
            if (!idle_tid) {
                task_t* idle_task = allocator.alloc_list;
                while (idle_task != NULL && idle_task->priority != 0) {
                    idle_task = idle_task->next_slab;
                }
                if (idle_task) {
                    idle_tid = idle_task->tid;
                }
            }

            // get perf stats
            uint64_t total_time = 0, kernel_time = 0, idle_time = 0, user_time = 0;
            for (int i = 0; i < performance_map.num_keys; ++i) {
                total_time += performance_map.values[i];
                if (performance_map.keys[i] == idle_tid) {
                    idle_time = performance_map.values[i];
                } else if (performance_map.keys[i] == kernel_task.tid) {
                    kernel_time = performance_map.values[i];
                }
            }

            // convert to percentage
            if (total_time > 0) {
                kernel_time = kernel_time * 100 / total_time;
                idle_time = idle_time * 100 / total_time;
                user_time = 100 - kernel_time - idle_time;
            }

            // print
            uart_printf(CONSOLE, "\033[s\033[1;1H\033[2Kkernel: %02u%%, idle: %02u%%, user: %02u%%\033[u", kernel_time, idle_time, user_time);

            // update last print time
            last_perf_print = timer_get_ms();
        }

        uint64_t syndrome = esr & 0xFFFF;

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
        case SYSCALL_CPUUSAGE: {
            cpu_usage_handler(&context);
            break;
        }
        case INTERRUPT_CODE: {
            handle_interrupt(&context);
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
