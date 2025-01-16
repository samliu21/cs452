#include "exception.h"
#include "rpi.h"
#include "stack.h"
#include "task.h"
#include "test.h"

void foo()
{
    uart_puts(CONSOLE, "Hello from foo!\r\n");
    __asm__("svc 5");
}

int kmain()
{
    gpio_init();
    uart_config_and_enable(CONSOLE);
    uart_puts(CONSOLE, "\r\nHello world!\r\n");

    tests();
    //char vbar[4 * 4 * 128];
    init_vbar(0, (uint64_t)&dummy_handler);
    char stack[NUM_TASKS * STACK_SIZE];
    task_t kernel_task;
    task_t task = task_new(priority_0, (uint64_t)stack, &foo);
    task_run(&kernel_task, &task);

    for (;;) {
    }
}
