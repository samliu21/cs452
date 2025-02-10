#ifndef _uart_notifier_h_
#define _uart_notifier_h_

#include "interrupt.h"
#include "name_server.h"
#include "syscall_func.h"
#include "uart_server.h"

void terminal_notifier()
{
    int64_t terminal_tid = who_is(TERMINAL_TASK_NAME);
    ASSERT(terminal_tid >= 0, "who_is failed");

    for (;;) {
        char c = await_event(EVENT_UART_TERMINAL);
        int64_t ret = send(terminal_tid, &c, 1, NULL, 0);
        ASSERT(ret >= 0, "send failed");
    }
}

void marklin_notifier()
{
    int64_t marklin_task_tid = who_is(MARKLIN_TASK_NAME);
    ASSERT(marklin_task_tid >= 0, "who_is failed");

    for (;;) {
        char c = await_event(EVENT_UART_MARKLIN);
        int64_t ret = send(marklin_task_tid, &c, 1, NULL, 0);
        ASSERT(ret >= 0, "send failed");
    }
}

#endif
