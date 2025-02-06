#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "interrupt.h"
#include "k3_user_tasks.h"
#include "name_server.h"
#include "syscall_func.h"
#include "uart_notifier.h"
#include "uart_server.h"

void k4_user_task()
{
    int64_t uart_server_tid = who_is("uart_server");
    ASSERT(uart_server_tid >= 0, "who_is failed");

    for (;;) {
        char c = getc(uart_server_tid, CONSOLE);
        putc(uart_server_tid, CONSOLE, c);
    }

    exit();
}

void k4_initial_user_task()
{
    create(1, &k2_name_server);
    create(0, &k3_idle_task);
    create(1, &k4_uart_server);
    create(1, &k4_uart_notifier);
    create(1, &k4_user_task);

    exit();
}

#endif
