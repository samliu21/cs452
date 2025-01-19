#include "exception.h"
#include "priority_queue.h"
#include "rpi.h"
#include "stack.h"
#include "syscall.h"
#include "task.h"
#include "test.h"

void test()
{
    uart_puts(CONSOLE, "Hello from test!\r\n");
    for (;;) { }
}

void foo()
{
    uart_puts(CONSOLE, "Hello from foo!\r\n");
    uint64_t tid = my_tid();
    uart_printf(CONSOLE, "tid: %u\r\n", tid);

    create((uint64_t)priority_1, (uint64_t)&test);

    for (;;) { }
}

task_t* my_create(allocator_t* allocator, char* stack, uint64_t priority, func_t entry_point)
{
    task_t* task = allocator_alloc(allocator);
    int offset = (task - allocator->slabs) / sizeof(task_t);
    uint64_t sp = (uint64_t)(stack + (offset + 1) * STACK_SIZE);
    task_new(task, priority, sp, entry_point);
    return task;
}

int kmain()
{
    gpio_init();
    uart_config_and_enable(CONSOLE);
    uart_puts(CONSOLE, "\r\nHello world!\r\n");

    tests();
    init_vbar();
    char stack[NUM_TASKS * STACK_SIZE];
    task_t kernel_task;

    task_t tasks[NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, NUM_TASKS);
    task_t* initial_task = my_create(&allocator, stack, priority_0, &foo);

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

        uint64_t syndrome = esr & SYNDROME_MASK;
        uart_printf(CONSOLE, "syscall with code: %u\r\n", syndrome);

        if (syndrome == SYSCALL_MYTID) {
            task->registers[0] = task->tid;
        } else if (syndrome == SYSCALL_CREATE) {
            priority_t priority = (priority_t)task->registers[0];
            func_t entry_point = (func_t)task->registers[1];
            task_t* new_task = my_create(&allocator, stack, priority, entry_point);
            new_task->parent_tid = task->tid;
            pq_add(&scheduler, new_task);
            // uart_printf(CONSOLE, "priority: %u, entry_point: %u\r\n", priority, entry_point);
        }
    }
}
