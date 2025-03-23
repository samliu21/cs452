#include "exception.h"
#include "rpi.h"
#include "syscall_asm.h"

void dummy_handler()
{
    uart_puts(CONSOLE, "syscall went to the dummy handler :(");
    for (;;) { }
}

void dummy_handler2()
{
    uart_puts(CONSOLE, "syscall went to the dummy handler 2 :(");
    for (;;) { }
}

void dummy_handler3(uint64_t esr)
{
    uint64_t exception_class = (esr >> 26) & 0x3F;
    uart_printf(CONSOLE, "memory error, exception class: %u\r\n", exception_class);
    for (int i = 23; i >= 0; --i) {
        uart_printf(CONSOLE, "%u", (esr >> i) & 1);
    }
    uart_printf(CONSOLE, "\r\n");
    for (;;) { }
}

void print_register(uint64_t reg)
{
    uart_printf(CONSOLE, "register: %u\r\n", reg);
}

void synchronous_kernel_error(uint64_t esr)
{
    uint64_t exception_class = (esr >> 26) & 0x3F;
    uint64_t elr = debug_register();
    uart_printf(CONSOLE, "sync kernel error, exception class: %u, ELR: %d\r\n", exception_class, elr);
    for (;;) { }
}
