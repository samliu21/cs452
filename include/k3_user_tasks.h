#ifndef _k3_user_tasks_
#define _k3_user_tasks_

#include "timer.h"

void k3_initial_user_task()
{
    uart_puts(CONSOLE, "k3_initial_user_task\n");

    timer_set_delay(20);

    for (;;) {
        // uart_puts(CONSOLE, "in loop\r\n");
    }
}

#endif
