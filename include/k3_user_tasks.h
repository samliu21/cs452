#ifndef _k3_user_tasks_
#define _k3_user_tasks_

#include "clock_notifier.h"
#include "clock_server.h"
#include "timer.h"

// void k3_user_task()
// {
//     uint64_t clock_server_tid = who_is("clock_server");
//     uint64_t curr_time = time(clock_server_tid);
//     uart_printf(CONSOLE, "time: %u\r\n", curr_time);

//     delay(clock_server_tid, 10);

//     curr_time = time(clock_server_tid);
//     uart_printf(CONSOLE, "time: %u\r\n", curr_time);

//     delay_until(clock_server_tid, 15);

//     curr_time = time(clock_server_tid);
//     uart_printf(CONSOLE, "time: %u\r\n", curr_time);

//     exit();
// }

void k3_client_task()
{
    uint64_t parent_tid = my_parent_tid();
    uint64_t tid = my_tid();
    uint64_t clock_server_tid = who_is("clock_server");

    char response[8];
    ASSERT(send(parent_tid, NULL, 0, response, 8) >= 0, "send failed");
    uint64_t delay_interval = a2ui(response, 10);
    uint64_t num_intervals = a2ui(response + 4, 10);

    for (uint64_t i = 0; i < num_intervals; ++i) {
        delay(clock_server_tid, delay_interval);
        uart_printf(CONSOLE, "tid: %u, delay: %u, intervals completed: %u\r\n", tid, delay_interval, i + 1);
    }

    exit();
}

void k3_idle_task()
{
    uart_puts(CONSOLE, "idle task begin\r\n");
    int iterations_per_print = 100;
    for (int i = 0;; ++i) {
        if (i % iterations_per_print == 0) {
            uint64_t usage = my_cpu_usage();
            uart_printf(CONSOLE, "\r\n", usage);
            uart_printf(CONSOLE, "Idle percentage: %u\r\n", usage);
            uart_printf(CONSOLE, "Idle percentage: %u\r\n", usage);
        }
    }
}

void k3_initial_user_task()
{
    // uart_puts(CONSOLE, "k3_initial_user_task\r\n");
    create(100, &k2_name_server);
    create(100, &k3_clock_server);
    create(100, &k3_clock_notifier);
    create(0, &k3_idle_task);

    create(10, &k3_client_task);
    create(9, &k3_client_task);
    create(8, &k3_client_task);
    create(7, &k3_client_task);
    uint64_t tid;
    receive_empty(&tid);
    reply(tid, "010 020", 8);
    receive_empty(&tid);
    reply(tid, "023 009", 8);
    receive_empty(&tid);
    reply(tid, "033 006", 8);
    receive_empty(&tid);
    reply(tid, "071 003", 8);

    exit();
}

#endif
