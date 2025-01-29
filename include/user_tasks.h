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

#define NUM_REPEATS 50000

int64_t get_num_bytes(char key)
{
    switch (key) {
    case 0:
        return 4;
    case 1:
        return 64;
    case 2:
        return 256;
    default:
        ASSERT(0, "unexpected number of bytes");
        return 0;
    }
}

void k2_perf_sender() // tid 3
{
    uint64_t tid;
    char args[2];
    receive(&tid, args, 2);
    reply_empty(tid);
    int64_t num_bytes = get_num_bytes(args[1]);
    uint64_t sender_tid = my_tid();
    uint64_t receiver_tid = (args[0] == 'S') ? sender_tid + 1 : sender_tid - 1;

    char send_buf[256], reply_buf[256];
    memset(send_buf, 0, 256);
    memset(reply_buf, 0, 256);
    yield();

    send_empty(1);

    for (int i = 0; i < NUM_REPEATS; ++i) {
        ASSERT(send(receiver_tid, send_buf, num_bytes, reply_buf, num_bytes) >= 0, "send failed");
    }

    send_empty(1);

    exit();
}

void k2_perf_receiver() // tid 4
{
    uint64_t tid;
    char args[2];
    receive(&tid, args, 2);
    reply_empty(tid);
    int64_t num_bytes = get_num_bytes(args[1]);

    uint64_t sender;
    char receive_buf[256], reply_buf[256];
    memset(receive_buf, 0, 256);
    memset(reply_buf, 0, 256);
    yield();

    send_empty(1);

    for (int i = 0; i < NUM_REPEATS; ++i) {
        ASSERT(receive(&sender, receive_buf, num_bytes) >= 0, "receive failed");

        ASSERT(reply(sender, reply_buf, num_bytes) >= 0, "reply failed");
    }

    exit();
}

void k2_perf_test()
{
    uint64_t tester_id;
    char args[2];
    int n = receive(&tester_id, args, 2);
    ASSERT(n = 2, "Did not receive enough args for perf test.\r\n");
    reply_empty(tester_id);
    uint64_t tid = 0, tid2 = 0;

    switch (args[0]) {
    case 'S': {
        tid = create(2, &k2_perf_sender);
        tid2 = create(2, &k2_perf_receiver);
        uart_printf(CONSOLE, "Testing Sender First, %u Bytes\r\n", get_num_bytes(args[1]));
        break;
    }
    case 'R': {
        tid = create(2, &k2_perf_receiver);
        tid2 = create(2, &k2_perf_sender);
        uart_printf(CONSOLE, "Testing Receiver First, %u Bytes\r\n", get_num_bytes(args[1]));
        break;
    }
    default: {
        ASSERTF(0, "First arg for perf test was not S or R.\r\n");
    }
    }

    send(tid, args, 2, NULL, 0);
    send(tid2, args, 2, NULL, 0);

    exit();
}

void k2_perf_initial_task()
{
    uint32_t start_time, end_time;

    char modes[] = "SR";
    uint64_t test_id;
    char args[2];
    for (int i = 0; i < 3; ++i) {
        args[1] = i; // Set n_bytes to 4, 64, 256

        for (int j = 0; j < 2; ++j) {
            args[0] = modes[j];

            test_id = create(999, &k2_perf_test);
            send(test_id, args, 2, NULL, 0);

            // receive commands to start timer (from both sender and receiver)
            uint64_t tid, tid2;
            receive_empty(&tid);
            receive_empty(&tid2);
            ASSERT(tid == tid2 - 1, "wrong order");
            start_time = timer_get_us();
            reply_empty(tid);
            reply_empty(tid2);

            // receive command to end timer
            receive_empty(&tid);
            end_time = timer_get_us();
            reply_empty(tid);

            // uart_printf(CONSOLE, "start: %u, end: %u\r\n", start_time, end_time);
            uint32_t avg_time = (end_time - start_time) / NUM_REPEATS;
            uart_printf(CONSOLE, "time taken: %u us\r\n", avg_time);
        }
    }
    exit();
}

#endif
