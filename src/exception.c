#include "rpi.h"
#include "exception.h"

void dummy_handler()
{
    uart_puts(CONSOLE, "Syscall went to the dummy handler :(");
}

void exception_handler()
{
    uart_puts(CONSOLE, "Syscall went to the exception handler :)");
}