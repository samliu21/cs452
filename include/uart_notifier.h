#ifndef _uart_notifier_h_
#define _uart_notifier_h_

#include "interrupt.h"
#include "name_server.h"
#include "syscall_func.h"
#include "uart_server.h"

void k4_uart_notifier()
{
    int64_t uart_server_tid = who_is("uart_server");
    ASSERT(uart_server_tid >= 0, "who_is failed");

    for (;;) {
        char c = await_event(EVENT_UART);
        send(uart_server_tid, &c, 1, NULL, 0);
    }
}

#endif
