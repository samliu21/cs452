#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "clock_server.h"
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
    puts(uart_server_tid, CONSOLE, "> ");
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
            puts(uart_server_tid, CONSOLE, "> ");
        } else {
            putc(uart_server_tid, CONSOLE, c);
        }
    }

    exit();
}

void train_speed_task()
{
    int64_t ret = register_as(TRAIN_SPEED_SERVER_NAME);
    ASSERT(ret >= 0, "register_as failed");
    int64_t uart_server_tid = who_is(MARKLIN_SERVER_NAME);
    ASSERT(uart_server_tid >= 0, "who_is failed");

    uint64_t caller_tid;
    char buf[2];
    for (;;) {
        receive(&caller_tid, buf, 2);
        char sendbuf[3];
        sendbuf[0] = buf[0] + 16;
        sendbuf[1] = buf[1];
        sendbuf[2] = 0;
        puts(uart_server_tid, MARKLIN, sendbuf);

        reply_empty(caller_tid);
    }

    exit();
}

void train_reverse_task()
{
    int64_t uart_server_tid = who_is(MARKLIN_SERVER_NAME);
    ASSERT(uart_server_tid >= 0, "who_is failed");
    int64_t clock_server_tid = who_is(CLOCK_SERVER_NAME);
    ASSERT(clock_server_tid >= 0, "who_is failed");

    uint64_t caller_tid;
    char train;
    receive(&caller_tid, &train, 1);
    reply_empty(caller_tid);

    int64_t res = delay(clock_server_tid, 500); // delay for 5s
    ASSERT(res >= 0, "delay failed");
    char sendbuf[3];
    sendbuf[0] = 0x1f;
    sendbuf[1] = train;
    sendbuf[2] = 0;
    puts(uart_server_tid, MARKLIN, sendbuf);

    exit();
}

void k4_state_task()
{
}

void k4_initial_user_task()
{
    create(1, &name_server_task);
    create(1, &clock_server_task);
    create(1, &clock_notifier_task);
    create(0, &idle_task);

    char c;
    uint64_t terminal_server_tid = create(1, &uart_server_task);
    c = CONSOLE;
    send(terminal_server_tid, &c, 1, NULL, 0);
    uint64_t marklin_server_tid = create(1, &uart_server_task);
    c = MARKLIN;
    send(marklin_server_tid, &c, 1, NULL, 0);

    create(1, &command_task);

    create(1, &terminal_notifier);
    create(1, &marklin_notifier);

    create(1, &train_speed_task);

    create(1, &terminal_task);

    exit();
}

#endif
