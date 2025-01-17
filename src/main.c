#include "exception.h"
#include "rpi.h"
#include "stack.h"
#include "task.h"
#include "test.h"
#include "syscall.h"

extern void Yield();
extern void default_registers_test();

void foo()
{
    uart_puts(CONSOLE, "Hello from foo!\r\n");
    leave_task();
    // uart_printf(CONSOLE, "tid: %u\r\n", tid);
    for (;;) {}
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
    task_t task = task_new(priority_0, (uint64_t)stack, &foo);
    default_registers_test();

    for (;;) {
        uint64_t esr = enter_task((uint64_t)&kernel_task, (uint64_t)&task);
        uint64_t syndrome = esr & SYNDROME_MASK;
        uart_printf(CONSOLE, "syscall with code: %u\r\n", syndrome);

        // if (syndrome == SYSCALL_MYTID) {
        //     task.registers[0] = task.tid;
        // }
    }
}
