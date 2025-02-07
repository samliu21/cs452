#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "interrupt.h"
#include "k3_user_tasks.h"
#include "name_server.h"
#include "syscall_func.h"
#include "uart_notifier.h"
#include "uart_server.h"

void k4_terminal_task()
{
    int64_t uart_server_tid = who_is("uart_terminal_server");
    ASSERT(uart_server_tid >= 0, "who_is failed");

    for (;;) {
        char c = getc(uart_server_tid, CONSOLE);
        if (c == '\r' || c == '\n') {
            puts(uart_server_tid, CONSOLE, "\r\n");
        } else {
            putc(uart_server_tid, CONSOLE, c);
        }
    }

    exit();
}

void k4_marklin_task()
{
    int64_t uart_server_tid = who_is("uart_marklin_server");
    ASSERT(uart_server_tid >= 0, "who_is failed");

    putc(uart_server_tid, MARKLIN, 26);
    putc(uart_server_tid, MARKLIN, 55);

    exit();
}

void k4_initial_user_task()
{
    create(1, &k2_name_server);
    create(0, &k3_idle_task);

    char c;
    uint64_t terminal_server_tid = create(1, &k4_uart_server);
    c = CONSOLE;
    send(terminal_server_tid, &c, 1, NULL, 0);
    uint64_t marklin_server_tid = create(1, &k4_uart_server);
    c = MARKLIN;
    send(marklin_server_tid, &c, 1, NULL, 0);

    create(1, &k4_terminal_notifier);
    create(1, &k4_marklin_notifier);

    create(1, &k4_terminal_task);
    create(1, &k4_marklin_task);

    exit();
}

#endif
