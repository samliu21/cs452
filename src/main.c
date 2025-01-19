#include "allocator.h"
#include "exception.h"
#include "priority_queue.h"
#include "rpi.h"
#include "stack.h"
#include "syscall.h"
#include "task.h"
#include "test.h"
#include "user_tasks.h"

int kmain()
{
    gpio_init();
    uart_config_and_enable(CONSOLE);
    uart_puts(CONSOLE, "\r\nconsole loaded!\r\n");

    // run tests and initialize exception vector
    tests();
    init_vbar();

    // create kernel task
    task_t kernel_task;
    kernel_task.tid = 0;

    // create allocator
    task_t tasks[NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, NUM_TASKS);
    char stack[NUM_TASKS * STACK_SIZE];

    // create initial task
    uint64_t n_tasks = 1;
    task_t* initial_task = allocator_new_task(&allocator, stack, n_tasks++, priority_0, &initial_user_task, &kernel_task);

    // create PQ with initial task in it
    priority_queue_t scheduler = pq_new();
    pq_add(&scheduler, initial_task);

    task_t* active_task;
    int schedule_new_task = 1;
    for (;;) {
        if (schedule_new_task) {
            active_task = pq_peek(&scheduler);
        }
        schedule_new_task = 1;

        uint64_t esr = enter_task((uint64_t)&kernel_task, (uint64_t)active_task);

        uint64_t syndrome = esr & 0xFFFF;
        uart_printf(CONSOLE, "syscall with code: %u\r\n", syndrome);

        switch (syndrome) {
        case SYSCALL_CREATE:
            priority_t priority = (priority_t)active_task->registers[0];
            func_t entry_point = (func_t)active_task->registers[1];
            task_t* new_task = allocator_new_task(&allocator, stack, n_tasks++, priority, entry_point, active_task);
            pq_add(&scheduler, new_task);
            break;
        case SYSCALL_MYTID:
            active_task->registers[0] = active_task->tid;
            schedule_new_task = 0;
            break;
        case SYSCALL_MYPARENTTID:
            active_task->registers[0] = active_task->parent_tid;
            schedule_new_task = 0;
            break;
        case SYSCALL_YIELD:
            pq_pop(&scheduler);
            pq_add(&scheduler, active_task);
            break;
        case SYSCALL_EXIT:
            pq_pop(&scheduler);
            break;
        default:
            uart_puts(CONSOLE, "unrecognized syscall\r\n");
            break;
        }
    }
}
