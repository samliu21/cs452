#ifndef _k3_user_tasks_
#define _k3_user_tasks_

#include "timer.h"

void k3_clock_notifier()
{
    uart_puts(CONSOLE, "notifier begin\r\n");

    for (;;) {
        await_event(EVENT_TICK);

        uart_printf(CONSOLE, "notifier end\r\n");
    }

    exit();
}

void k3_idle_task()
{
    uart_puts(CONSOLE, "idle task begin\r\n");

    for (;;) { }
}

void k3_initial_user_task()
{
    uart_puts(CONSOLE, "k3_initial_user_task\r\n");
    create(0, &k3_idle_task);
    create(5, &k3_clock_notifier);
    exit();
}

#endif
