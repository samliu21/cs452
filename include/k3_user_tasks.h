#ifndef _k3_user_tasks_
#define _k3_user_tasks_

#include "clock_notifier.h"
#include "clock_server.h"
#include "timer.h"

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
    for (;;) {
        __asm__ volatile("wfi");
    }
}

void k3_initial_user_task()
{
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
