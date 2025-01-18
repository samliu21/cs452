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
    int tid = 0;
    my_tid();
    uart_printf(CONSOLE, "tid: %u\r\n", tid);
    
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
    task_t task = task_new(priority_0, (uint64_t)(stack + STACK_SIZE), &foo);
    default_registers_test();

    for (;;) {
        uint64_t esr = enter_task((uint64_t)&kernel_task, (uint64_t)&task);
        uint64_t syndrome = esr & SYNDROME_MASK;
        uart_printf(CONSOLE, "syscall with code: %u\r\n", syndrome);

        uint64_t registers[32];
        dump_registers((uint64_t)registers);
        for (int i = 0; i < 32; ++i) {
            uart_printf(CONSOLE, "register %d: %u\r\n", i, registers[i]);
        }
        while(1){}

        if (syndrome == SYSCALL_MYTID) {
            task.registers[0] = task.tid;
        }
    }
}
