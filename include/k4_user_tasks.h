#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "command.h"
#include "interrupt.h"
#include "k3_user_tasks.h"
#include "marklin.h"
#include "name_server.h"
#include "sensor.h"
#include "state_server.h"
#include "syscall_func.h"
#include "uart_notifier.h"
#include "uart_server.h"

void terminal_task()
{
    int64_t uart_task_tid = who_is(TERMINAL_TASK_NAME);
    ASSERT(uart_task_tid >= 0, "who_is failed");
    int64_t command_task_tid = who_is(COMMAND_TASK_NAME);
    ASSERT(command_task_tid >= 0, "who_is failed");

    char command[32], command_result_buf[32];
    int command_pos = 0;
    puts(uart_task_tid, CONSOLE, "> ");
    for (;;) {
        char c = getc(uart_task_tid, CONSOLE);
        command[command_pos++] = c;
        if (c == '\r' || c == '\n') {
            puts(uart_task_tid, CONSOLE, "\r\n");

            command[command_pos] = 0;
            send(command_task_tid, command, command_pos + 1, command_result_buf, 32);

            command_result_t* command_result = (command_result_t*)command_result_buf;
            if (command_result->type == COMMAND_QUIT) {
                // return 0;
            } else if (command_result->type == COMMAND_FAIL) {
                printf(uart_task_tid, CONSOLE, "error: %s\r\n", command_result->error_message);
            }

            command_pos = 0;
            puts(uart_task_tid, CONSOLE, "> ");
        } else {
            putc(uart_task_tid, CONSOLE, c);
        }
    }

    exit();
}

void display_state_task()
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");
    int64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    ASSERT(clock_task_tid >= 0, "who_is failed");
    int64_t terminal_task_tid = who_is(TERMINAL_TASK_NAME);
    ASSERT(terminal_task_tid >= 0, "who_is failed");

    for (;;) {
        char sensors[32];
        state_get_recent_sensors(state_task_tid, sensors);

        // puts(CONSOLE, CONSOLE, "\033[s\033[2;1H\033[2Ksensors\033[u");
        uart_printf(CONSOLE, "\033[s\033[2;1H\033[2Ksensors: %s\r\n\033[u", sensors);

        delay(clock_task_tid, 5);
    }
}

void k4_initial_user_task()
{
    // system tasks
    create(1, &name_server_task);
    create(1, &clock_server_task);
    create(1, &clock_notifier_task);
    create(0, &idle_task);

    // IO tasks
    char c;
    uint64_t terminal_task_tid = create(1, &uart_server_task);
    c = CONSOLE;
    send(terminal_task_tid, &c, 1, NULL, 0);
    uint64_t marklin_task_tid = create(1, &uart_server_task);
    c = MARKLIN;
    send(marklin_task_tid, &c, 1, NULL, 0);
    create(1, &terminal_notifier);
    create(1, &marklin_notifier);

    // train setup tasks
    create(1, &state_task);
    create(1, &sensor_task);
    create(1, &display_state_task);

    // client tasks
    create(1, &command_task);
    create(1, &terminal_task);

    exit();
}

#endif
