#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "command.h"
#include "interrupt.h"
#include "k3_user_tasks.h"
#include "name_server.h"
#include "syscall_func.h"
#include "uart_notifier.h"
#include "uart_server.h"

void terminal_task()
{
    int64_t uart_server_tid = who_is(TERMINAL_SERVER_NAME);
    ASSERT(uart_server_tid >= 0, "who_is failed");
    int64_t command_server_tid = who_is(COMMAND_SERVER_NAME);
    ASSERT(command_server_tid >= 0, "who_is failed");

    char command[32], command_result_buf[32];
    int command_pos = 0;
    for (;;) {
        char c = getc(uart_server_tid, CONSOLE);
        command[command_pos++] = c;
        if (c == '\r' || c == '\n') {
            puts(uart_server_tid, CONSOLE, "\r\n");

            command[command_pos] = 0;
            send(command_server_tid, command, command_pos + 1, command_result_buf, 32);

            command_result_t* command_result = (command_result_t*)command_result_buf;
            if (command_result->type == COMMAND_QUIT) {
                // return 0;
            } else if (command_result->type == COMMAND_FAIL) {
                printf(uart_server_tid, CONSOLE, "error: %s\r\n", command_result->error_message);
            }

            command_pos = 0;
        } else {
            putc(uart_server_tid, CONSOLE, c);
        }
    }

    exit();
}

void marklin_task()
{
    int64_t uart_server_tid = who_is(MARKLIN_SERVER_NAME);
    ASSERT(uart_server_tid >= 0, "who_is failed");

    // putc(uart_server_tid, MARKLIN, 26);
    // putc(uart_server_tid, MARKLIN, 55);

    exit();
}

void k4_initial_user_task()
{
    create(1, &name_server_task);
    create(0, &idle_task);

    char c;
    uint64_t terminal_server_tid = create(1, &k4_uart_server);
    c = CONSOLE;
    send(terminal_server_tid, &c, 1, NULL, 0);
    uint64_t marklin_server_tid = create(1, &k4_uart_server);
    c = MARKLIN;
    send(marklin_server_tid, &c, 1, NULL, 0);

    create(1, &k4_command_server);

    create(1, &k4_terminal_notifier);
    create(1, &k4_marklin_notifier);

    create(1, &terminal_task);
    create(1, &marklin_task);

    exit();
}

#endif
