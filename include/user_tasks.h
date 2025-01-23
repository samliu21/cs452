#ifndef _user_tasks_
#define _user_tasks_

#include "common.h"
#include "stringmap.h"
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
#define WHO_IS 'W'
#define REGISTER_AS 'R'

void k2_initial_user_task() { }

void k2_name_server()
{
    stringmap_t names;
    uint64_t tid, mapped_tid;
    const int bufsize = 128;
    char namebuf[bufsize + 1];
    char argv[3][MAX_KEY_SIZE];

    for (;;) {
        int64_t sz = receive(&tid, namebuf, bufsize);
        namebuf[sz] = 0;
        int argc = split(namebuf, (char**)argv);

        switch (argv[0][0]) {
        case WHO_IS:
            ASSERT(argc == 2, "'who_is' takes 1 argument");
            mapped_tid = stringmap_get(&names, argv[1]);
            reply_uint(tid, mapped_tid);
            break;
        case REGISTER_AS:
            ASSERT(argc == 3, "'register_as' takes 2 arguments");
            mapped_tid = a2ui(argv[2], 10);
            stringmap_set(&names, argv[1], mapped_tid);
            reply_uint(tid, 0);
            break;
        default:
            ASSERT(0, "invalid command");
        }
    }
}

void k2_rps_server() { }

void k2_rps_client() { }

#endif
