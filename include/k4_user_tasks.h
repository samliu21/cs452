#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "interrupt.h"
#include "k3_user_tasks.h"
#include "syscall_func.h"

void k4_uart_handler()
{
    for (;;) {
        await_event(EVENT_UART);
    }

    exit();
}

void k4_initial_user_task()
{
    create(0, &k3_idle_task);
    create(1, &k4_uart_handler);

    exit();
}

#endif
