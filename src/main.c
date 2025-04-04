#include "allocator.h"
#include "common.h"
#include "exception.h"
#include "interrupt.h"
#include "k2_perf_test.h"
#include "k2_user_tasks.h"
#include "k3_user_tasks.h"
#include "k4_user_tasks.h"
#include "priority_queue_task.h"
#include "queue_pi.h"
#include "rpi.h"
#include "syscall_asm.h"
#include "syscall_handler.h"
#include "task.h"
#include "test.h"
#include "track_algo.h"
#include "track_data.h"

extern void setup_mmu();

#define PRINT_PERF_AFTER_MS 50
#define MAX_QUEUE_NODES 100000

int kmain()
{
#if defined(MMU)
    setup_mmu();
#endif

    gpio_init();
    uart_config_and_enable(CONSOLE);
    uart_config_and_enable(MARKLIN);

    // track_path_t path = track_path_new();
    // track_node track[TRACK_MAX];
    // init_tracka(track);
    // int x = get_next_segments(track, &path, 130, 750);
    // uart_printf(CONSOLE, "x: %d\r\n", x);
    // for (int i = 0; i < path.path_length; ++i) {
    //     uart_printf(CONSOLE, "path[%d]: %s\r\n", i, track[path.nodes[i]].name);
    // }
    // x = get_next_segments(track, &path, 1, 750);
    // uart_printf(CONSOLE, "x: %d\r\n", x);
    // for (int i = 0; i < path.path_length; ++i) {
    //     uart_printf(CONSOLE, "path[%d]: %s\r\n", i, track[path.nodes[i]].name);
    // }
    // for (;;) { }

    // run tests and initialize exception vector
    init_interrupts();
    init_vbar();
    int failed = tests();
    if (failed) {
        for (;;) { }
    }

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
    task_t* initial_task = allocator_new_task(&allocator, stack, n_tasks++, 1, &k4_initial_user_task, &kernel_task);
#endif
    // create PQ with initial task in it
    priority_queue_task_t scheduler = pq_task_new();
    pq_task_add(&scheduler, initial_task);

    // blocked queues
    queue_task_t tasks_waiting_for_send = queue_task_new();
    queue_task_t tasks_waiting_for_reply = queue_task_new();
    queue_task_t tasks_waiting_for_timer = queue_task_new();
    queue_task_t tasks_waiting_for_terminal = queue_task_new();
    queue_task_t tasks_waiting_for_marklin = queue_task_new();

    // timer
    uint32_t next_tick = timer_get_us() + US_PER_TICK;

    // performance metrics
    uint64_t kernel_time_start = timer_get_us(), kernel_time_end;

    pi_t* kernel_time_nodes = (pi_t*)(USER_STACK_START + NUM_TASKS * STACK_SIZE);
    pi_t* idle_time_nodes = kernel_time_nodes + MAX_QUEUE_NODES;
    int kernel_next_node = 0;
    int idle_next_node = 0;

    queue_pi_t kernel_time_queue = queue_pi_new();
    queue_pi_t idle_time_queue = queue_pi_new();

    // kernel context
    main_context_t context;
    context.kernel_task = &kernel_task;
    context.allocator = &allocator;
    context.stack = stack;
    context.n_tasks = &n_tasks;
    context.scheduler = &scheduler;
    context.tasks_waiting_for_send = &tasks_waiting_for_send;
    context.tasks_waiting_for_reply = &tasks_waiting_for_reply;
    context.tasks_waiting_for_timer = &tasks_waiting_for_timer;
    context.tasks_waiting_for_terminal = &tasks_waiting_for_terminal;
    context.tasks_waiting_for_marklin = &tasks_waiting_for_marklin;
    context.next_tick = next_tick;
    context.kernel_time_queue = &kernel_time_queue;
    context.idle_time_queue = &idle_time_queue;
    context.kernel_time = 0;
    context.idle_time = 0;
    context.kernel_execution_start_time = timer_get_us();

    while (!pq_task_empty(&scheduler)) {
        context.active_task = pq_task_pop(&scheduler);
        ASSERT(context.active_task->state == READY, "active task is not in ready state");

        kernel_time_end = timer_get_us();

        pi_t* kernel_time_node = &(kernel_time_nodes[kernel_next_node++]);
        if (kernel_next_node >= MAX_QUEUE_NODES)
            kernel_next_node = 0;
        kernel_time_node->weight = kernel_time_start; // weight = start time
        kernel_time_node->id = kernel_time_end - kernel_time_start;
        context.kernel_time += kernel_time_node->id;
        queue_pi_add(&kernel_time_queue, kernel_time_node);
        ASSERTF(kernel_time_queue.size < MAX_QUEUE_NODES, "kernel ran out of time nodes.");

        uint64_t esr = enter_task(&kernel_task, context.active_task);

        kernel_time_start = timer_get_us();
        if (context.active_task->priority == 0) {
            pi_t* idle_time_node = &(idle_time_nodes[idle_next_node++]);
            if (idle_next_node >= MAX_QUEUE_NODES)
                idle_next_node = 0;
            idle_time_node->weight = kernel_time_end; // weight = start time
            idle_time_node->id = kernel_time_start - kernel_time_end;
            context.idle_time += idle_time_node->id;
            queue_pi_add(&idle_time_queue, idle_time_node);
            ASSERTF(idle_time_queue.size < MAX_QUEUE_NODES, "idle ran out of time nodes.");
        }

        uint64_t syndrome = esr & 0xFFFF;
        if (syndrome != INTERRUPT_CODE) {
            int type = (esr >> 26) & 0x3F;
            uint64_t elr = debug_register();
            ASSERTF(type == 21, "not an svc call; type: %d, ELR: %d\r\n", type, elr);
        }

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
            if (context.active_task->tid == 10) {
                pq_task_add(&scheduler, context.active_task);
                break;
            }
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
        case SYSCALL_CPU_USAGE: {
            cpu_usage_handler(&context);
            break;
        }
        case SYSCALL_TERMINATE: {
            return 0;
        }
        case INTERRUPT_CODE: {
            handle_interrupt(&context);
            break;
        }
        default: {
            ASSERTF(0, "unrecognized syscall: %u\r\n", syndrome);
            for (;;) { }
        }
        }

        if (syndrome != SYSCALL_EXIT && context.active_task->state == READY) {
            pq_task_add(&scheduler, context.active_task);
        }
    }

    uart_puts(CONSOLE, "No tasks to run\r\n");
    for (;;) { }
}
