#ifndef _k4_user_tasks_
#define _k4_user_tasks_

#include "command.h"
#include "interrupt.h"
#include "k3_user_tasks.h"
#include "marklin.h"
#include "name_server.h"
#include "sensor.h"
#include "state_server.h"
#include "switch.h"
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
            int64_t ret = send(command_task_tid, command, command_pos + 1, command_result_buf, 32);
            ASSERT(ret >= 0, "send failed");

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
    int64_t marklin_task_tid = who_is(MARKLIN_TASK_NAME);
    ASSERT(marklin_task_tid >= 0, "who_is failed");

    // show loading text while switches are being set
    puts(terminal_task_tid, CONSOLE, "\033[s\033[1;1H\033[2KSetting up trains...\033[u");

    // set switches to straight
    tswitch_t switch_buf[64];
    switchlist_t switchlist = switch_createlist(switch_buf);
    for (int i = 0; i < switchlist.n_switches; ++i) {
        marklin_set_switch(marklin_task_tid, switchlist.switches[i].id, 'S');
    }
    int64_t ret = create(1, &deactivate_solenoid_task);
    ASSERT(ret >= 0, "create failed");

    int64_t ticks = time(clock_task_tid);
    for (;;) {
        // perf
        uint64_t usage = cpu_usage();
        uint64_t kernel_percentage = usage % 100;
        uint64_t idle_percentage = usage / 100;
        uint64_t user_percentage = 100 - kernel_percentage - idle_percentage;
        char* format = "\033[s\033[1;1H\033[2Kkernel: %02u%%, idle: %02u%%, user: %02u%%\033[u";
        printf(terminal_task_tid, CONSOLE, format, kernel_percentage, idle_percentage, user_percentage);

        // clock
        uint64_t ticks = time(clock_task_tid);
        uint64_t minutes = ticks / 6000;
        uint64_t seconds = (ticks % 6000) / 100;
        uint64_t tenths = (ticks % 100) / 10;
        printf(terminal_task_tid, CONSOLE, "\033[s\033[2;1H\033[2K%02d:%02d:%02d\033[u", minutes, seconds, tenths);

        // sensors
        char sensors[32];
        state_get_recent_sensors(state_task_tid, sensors);
        printf(terminal_task_tid, CONSOLE, "\033[s\033[3;1H\033[2Ksensors: [ %s]\033[u", sensors);

        // switches
        char switches[128];
        state_get_switches(state_task_tid, switches);
        printf(terminal_task_tid, CONSOLE, "\033[s\033[4;1H\033[2Kswitches: [ %s]\033[u", switches);

        // print every 50ms
        ticks += 5;
        ret = delay_until(clock_task_tid, ticks);
        ASSERT(ret >= 0, "delay failed");
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
    ASSERT(ret >= 0, "send failed");
    uint64_t marklin_task_tid = create(1, &uart_server_task);
    c = MARKLIN;
    ret = send(marklin_task_tid, &c, 1, NULL, 0);
    ASSERT(ret >= 0, "send failed");
    create(1, &terminal_notifier);
    create(1, &marklin_notifier);

    // clear screen
    puts(terminal_task_tid, CONSOLE, "\033[2J\033[999;1H");

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
