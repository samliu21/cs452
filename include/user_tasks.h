#ifndef _user_tasks_
#define _user_tasks_

#include "common.h"
#include "name_server.h"
#include "rps_server.h"
#include "stringmap.h"
#include "syscall_func.h"
#include "timer.h"
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

uint32_t start_time, end_time;
char send_buf[256], receive_buf[256], reply_buf[256];
uint32_t num_bytes;
const int NUM_REPEATS = 8092;

void reset_timers()
{
    start_time = 0;
    end_time = 0;
    memset(send_buf, 0, 256);
    memset(receive_buf, 0, 256);
    memset(reply_buf, 0, 256);
}

void start_timer()
{
    if (!start_time) {
        start_time = timer_get_us();
    }
}

void end_timer()
{
    if (!end_time) {
        end_time = timer_get_us();
    }
}

void k2_perf_sender() // tid 3
{
    start_timer();

    for (int i = 0; i < NUM_REPEATS; ++i) {
        ASSERT(send(4, send_buf, num_bytes, reply_buf, num_bytes) >= 0, "send failed");
    }

    end_timer();

    exit();
}

void k2_perf_receiver() // tid 4
{
    uint64_t sender;

    start_timer();

    for (int i = 0; i < NUM_REPEATS; ++i) {
        ASSERT(receive(&sender, send_buf, num_bytes) >= 0, "receive failed");

        ASSERT(reply(sender, reply_buf, num_bytes) >= 0, "reply failed");
    }

    end_timer();

    exit();
}

void k2_perf_test()
{
    uart_printf(CONSOLE, "testing perf of SRR...\r\n");
    uint64_t tester_id;
    char args[2];
    int n = receive(&tester_id, args, 2);
    ASSERT(n = 2, "Did not receive enough args for perf test.\r\n");
    reply_empty(tester_id);
    reset_timers();
    num_bytes = (uint32_t)args[1];
    switch (args[0]) {
    case 'S': {
        create(2, &k2_perf_sender);
        create(2, &k2_perf_receiver);
        uart_printf(CONSOLE, "Testing Sender First, %d Bytes\r\n", args[1]);
        break;
    }
    case 'R': {
        create(2, &k2_perf_receiver);
        create(2, &k2_perf_sender);
        uart_printf(CONSOLE, "Testing Receiver First, %d Bytes\r\n", args[1]);
        break;
    }
    default: {
        ASSERTF(0, "First arg for perf test was not S or R.\r\n");
    }
    }

    uart_puts(CONSOLE, "done creating tasks; run them now\r\n");
    exit();
}

void k2_perf_initial_task()
{
    reset_timers();
    start_timer();
    end_timer();
    uint32_t overhead = end_time - start_time;

    char modes[] = "SR";
    uint64_t test_id;
    char args[2];
    args[1] = 1;
    for (int i = 0; i < 3; ++i) {
        args[1] *= 4; // Set n_bytes to 4, 64, 256

        for (int j = 0; j < 2; ++j) {
            args[0] = modes[j];

            test_id = create(999, &k2_perf_test);
            send(test_id, args, 2, NULL, 0);
            uart_printf(CONSOLE, "start: %u, end: %u\r\n", start_time, end_time);
            uint32_t avg_time = (end_time - start_time - overhead) / NUM_REPEATS;
            uart_printf(CONSOLE, "time taken: %u us\r\n", avg_time);
        }
    }
    exit();
}

#endif
