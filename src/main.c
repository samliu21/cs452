#include "rpi.h"
#include "stack.h"
#include "task.h"
#include "test.h"

void foo()
{
    uart_puts(CONSOLE, "Hello from foo!\r\n");
}

int kmain()
{
    gpio_init();
    uart_config_and_enable(CONSOLE);
    uart_puts(CONSOLE, "\r\nHello world!\r\n");

    tests();

    char stack[NUM_TASKS * STACK_SIZE];
    task_t kernel_task;
    task_t task = task_new(priority_0, (uint64_t)stack, &foo);
    task_run(&kernel_task, &task);

    for (;;) {
    }
}
