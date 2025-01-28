#include "exception.h"
#include "rpi.h"

void dummy_handler()
{
    uart_puts(CONSOLE, "syscall went to the dummy handler :(");
    for (;;) { }
}

void print_register(uint64_t reg)
{
    uart_printf(CONSOLE, "register: %u\r\n", reg);
}

void synchronous_kernel_error(uint64_t esr)
{
    uint64_t exception_class = (esr >> 26) & 0x3F;
    uart_printf(CONSOLE, "sync kernel error, exception class: %u\r\n", exception_class);
    for (;;) { }
}
