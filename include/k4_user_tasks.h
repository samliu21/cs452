#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "command.h"
#include "display_state_server.h"
#include "interrupt.h"
#include "k3_user_tasks.h"
#include "marklin.h"
#include "name_server.h"
#include "sensor.h"
#include "state_server.h"
#include "switch.h"
#include "syscall_func.h"
#include "train.h"
#include "uart_notifier.h"
#include "uart_server.h"

arrow_key_t parse_arrow_key(char* c)
{
    if (!strcmp(c, "\033[A")) {
        return UP;
    }
    if (!strcmp(c, "\033[B")) {
        return DOWN;
    }
    if (!strcmp(c, "\033[C")) {
        return RIGHT;
    }
    if (!strcmp(c, "\033[D")) {
        return LEFT;
    }
    return 0;
}

void terminal_task()
{
    int64_t command_task_tid = who_is(COMMAND_TASK_NAME);
    ASSERT(command_task_tid >= 0, "who_is failed");

    char command[32];
    memset(command, 0, 32);
    command_result_t command_result;
    int command_pos = 0;
    // wait for servers to init
    display_lazy();
    for (;;) {
        char c = getc(CONSOLE);
        command[command_pos++] = c;
        if (command_pos >= 3) {
            arrow_key_t key = parse_arrow_key(&command[command_pos - 3]);
            if (key) {
                command_pos -= 3;
                memset(&command[command_pos], 0, 3);
                char player_command[5] = "pc x";
                player_command[3] = key;
                int64_t ret = send(command_task_tid, player_command, 5, NULL, 0);
                ASSERT(ret >= 0, "player command send failed");
                continue;
            }
        }
        if (c == '\r' || c == '\n') {
            puts(CONSOLE, "\r\n");
            display_force();

            int64_t ret = send(command_task_tid, command, command_pos + 1, (char*)&command_result, sizeof(command_result_t));
            ASSERT(ret >= 0, "command enter send failed");

            if (command_result.type == COMMAND_QUIT) {
                terminate();
            } else if (command_result.type == COMMAND_FAIL) {
                printf(CONSOLE, "error: %s\r\n", command_result.error_message);
                display_force();
            }

            command_pos = 0;
            memset(command, 0, 32);
            puts(CONSOLE, "\033[999;1H> ");

        } else if (c == '\b') {
            command[--command_pos] = 0;
            if (command_pos > 0) {
                puts(CONSOLE, "\033[D \033[D");
                command[--command_pos] = 0;
            }
        } else {
            putc(CONSOLE, c);
        }
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
    int64_t ret = send(terminal_task_tid, &c, 1, NULL, 0);
    ASSERT(ret >= 0, "terminal task startup send failed");
    uint64_t marklin_task_tid = create(1, &uart_server_task);
    c = MARKLIN;
    ret = send(marklin_task_tid, &c, 1, NULL, 0);
    ASSERT(ret >= 0, "marklin task startup send failed");
    create(1, &terminal_notifier);
    create(1, &marklin_notifier);

    // clear screen
    puts(CONSOLE, "\033[2J\033[91;999r\033[999;1H> ");

    // train setup tasks
    create(2, &train_task);
    create(1, &train_model_notifier);

    create(1, &state_task);
    create(1, &display_state_task);

    create(1, &display_state_notifier);
    create(1, &sensor_task);

    // client tasks
    create(1, &command_task);
    create(1, &player_actable_notifier);
    create(1, &terminal_task);

    exit();
}

#endif
