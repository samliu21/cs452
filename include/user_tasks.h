#ifndef _user_tasks_
#define _user_tasks_

#include "common.h"
#include "name_server.h"
#include "rps_server.h"
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

void k2_rps_client()
{
    ASSERT(signup() >= 0, "signup failed");
    for (int i = 0; i < 5; ++i) {
        rps_move_t move = (rps_move_t)((5 - i) % 3);
        ASSERT(play(move) != FAILED, "play failed");
    }

    exit();
}

void k2_rps_client_2()
{
    ASSERT(signup() >= 0, "signup failed");
    for (int i = 0; i < 5; ++i) {
        rps_move_t move = (rps_move_t)(i % 3);
        ASSERT(play(move) != FAILED, "play failed");
    }

    exit();
}

void k2_rps_client_random()
{
    int num_signups = myrand() % 2 + 1; // 1 or 2 signups
    for (int i = 0; i < num_signups; ++i) {
        ASSERT(signup() >= 0, "signup failed");

        int num_plays = myrand() % 3 + 1; // 1 to 3 plays
        for (int j = 0; j < num_plays; ++j) {
            rps_move_t move = (rps_move_t)(myrand() % 3);
            rps_server_response_t ret = play(move);
            ASSERT(ret != FAILED, "play failed");
            if (ret == ABANDONED) {
                break;
            }
        }

        ASSERT(quit() >= 0, "quit failed");
    }
    exit();
}

void press_enter_to_continue()
{
    uart_printf(CONSOLE, "\r\nThe end of a test... Press enter to continue.\r\n\r\n");
    char c;
    while (1) {
        c = uart_getc(CONSOLE);
        if (c == '\r' || c == '\n') {
            break;
        }
    }
}

void k2_priority_test()
{
    uart_printf(CONSOLE, "Testing 2 RPS clients, with priorities 2 and 3.\r\n");
    create(2, &k2_rps_client);
    create(3, &k2_rps_client_2);
}

void k2_random_test()
{
    uart_printf(CONSOLE, "Signing up 5 clients... Each client signs up either once or twice, and makes between 1 to 3 moves.\r\n");
    for (int i = 0; i < 5; ++i) {
        create(2, &k2_rps_client_random);
    }
}

void k2_initial_user_task()
{
    // create name server
    create(2, &k2_name_server);

    // create RPS server
    create(2, &k2_rps_server);

    // test with 2 RPS clients, with priorities 2 and 3
    k2_priority_test();
    press_enter_to_continue();

    // test that randomly spawns clients
    k2_random_test();
    press_enter_to_continue();

    exit();
}

#endif
