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
    uart_puts(CONSOLE, "\r\nhello world!\r\n");

    tests();
    init_vbar();
    char stack[NUM_TASKS * STACK_SIZE];
    task_t kernel_task;
    kernel_task.tid = 0;

    task_t tasks[NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, NUM_TASKS);
    uint64_t n_tasks = 1;
    task_t* initial_task = allocator_new_task(&allocator, stack, n_tasks++, priority_0, &foo, &kernel_task);

    priority_queue_t scheduler = pq_new();
    pq_add(&scheduler, initial_task);

    for (;;) {
        task_t* task = pq_peek(&scheduler);

        // debug_set_registers();
        uint64_t esr = enter_task((uint64_t)&kernel_task, (uint64_t)task);
        // debug_dump_registers((uint64_t)registers);
        // for (int i = 0; i < 32; ++i) {
        //     uart_printf(CONSOLE, "register %d: %u\r\n", i, registers[i]);
        // }
        // for (;;)
        //     ;

        uint64_t syndrome = esr & 0xFFFF;
        uart_printf(CONSOLE, "syscall with code: %u\r\n", syndrome);

        if (syndrome == SYSCALL_MYTID) {
            task->registers[0] = task->tid;
        } else if (syndrome == SYSCALL_CREATE) {
            priority_t priority = (priority_t)task->registers[0];
            func_t entry_point = (func_t)task->registers[1];
            task_t* new_task = allocator_new_task(&allocator, stack, n_tasks++, priority, entry_point, task);
            pq_add(&scheduler, new_task);
            // uart_printf(CONSOLE, "priority: %u, entry_point: %u\r\n", priority, entry_point);
        } else if (syndrome == SYSCALL_MYPARENTTID) {
            task->registers[0] = task->parent_tid;
        }
    }
}
