#ifndef _user_tasks_
#define _user_tasks_

#include "common.h"
#include "name_server.h"
#include "stringmap.h"
#include "syscall_func.h"
#include "util.h"

// K1
void k1_spawned_task()
{
    uint64_t tid, parent_tid;
    tid = my_tid();
    parent_tid = my_parent_tid();
    uart_printf(CONSOLE, "Spawned task has tid: %u, parent tid: %u\r\n", tid, parent_tid);
    yield();
    uart_printf(CONSOLE, "Spawned task has tid: %u, parent tid: %u\r\n", tid, parent_tid);
    exit();
}

void k1_initial_user_task()
{
    int64_t tid;
    tid = create(0, &k1_spawned_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid);
    tid = create(0, &k1_spawned_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid);
    tid = create(2, &k1_spawned_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid);
    tid = create(2, &k1_spawned_task);
    uart_printf(CONSOLE, "Created: %u\r\n", tid);
    uart_printf(CONSOLE, "FirstUserTask: exiting\r\n");
    exit();
}

// K2

void k2_initial_user_task()
{
    create(1, &k2_name_server);

    register_as("initial_task");
    for (;;) { }
}

void k2_rps_client() { }

#endif
