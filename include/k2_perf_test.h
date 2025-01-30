
#ifndef _k2_perf_test_
#define _k2_perf_test_

#include "common.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include "timer.h"

#define NUM_REPEATS 10000

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

void k2_perf_sender()
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

void k2_perf_receiver()
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
