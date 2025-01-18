#include "rpi.h"
#include "exception.h"

void dummy_handler()
{
    uart_puts(CONSOLE, "Syscall went to the dummy handler :(");
    for (;;) {}
}

void exception_handler()
{
    uart_puts(CONSOLE, "Syscall went to the exception handler :)");
    for (;;) {}
}

void print_hello()
{
    uart_puts(CONSOLE, "Hello\r\n");
}

void print_register(uint64_t reg)
{
    uart_printf(CONSOLE, "Register: %u\r\n", reg);
}


void synchronous_kernel_error(uint64_t esr) {
    uart_puts(CONSOLE, "Kernel Error\r\n");
    synchronous_error(esr);
}

void synchronous_user_error(uint64_t esr) {
    uart_puts(CONSOLE, "User Error\r\n");
    synchronous_error(esr);
}

void synchronous_error(uint64_t esr) {
    uint64_t exception_class = (esr >> 26) & 0x3F;
    uart_printf(CONSOLE, "Exception Class %u", exception_class);
    //uint64_t specific_
    for (;;) {}
}