#include "terminal.h"
#include "rpi.h"

void terminal_clear()
{
    uart_printf(CONSOLE, "\033[2J");
}

void terminal_set_cursor(int r, int c)
{
    uart_printf(CONSOLE, "\033[%d;%dH", r, c);
}

void terminal_set_cursor_origin()
{
    terminal_set_cursor(1, 1);
}

void terminal_clear_line()
{
    uart_printf(CONSOLE, "\033[K");
}

void terminal_save_cursor()
{
    uart_printf(CONSOLE, "\033[s");
}

void terminal_restore_cursor()
{
    uart_printf(CONSOLE, "\033[u");
}
