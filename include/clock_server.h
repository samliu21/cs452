#ifndef _clock_server_h_
#define _clock_server_h_

#include "common.h"
#include "name_server.h"
#include "queue.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include "timer.h"
#include "uintmap.h"

typedef enum {
    TIME = 'T',
    DELAY = 'D',
    DELAY_UNTIL = 'U',
    TICK = 'K',
} clock_server_request_t;

typedef enum {
    OK = 'O',
    NEGATIVE = 'N',
} clock_server_response_t;

int64_t time(uint64_t tid)
{
    uint64_t clock_server_tid = who_is("clock_server");
    if (tid != clock_server_tid) {
        return -1;
    }
    char request[2];
    request[0] = TIME;
    request[1] = 0;

    char response[16];
    int64_t ret = send(clock_server_tid, request, 2, response, 16);

    if (ret < 0) {
        return -1;
    } else {
        return a2ui(response, 10);
    }
}

int64_t delay(uint64_t tid, int64_t ticks)
{
    uint64_t clock_server_tid = who_is("clock_server");
    if (tid != clock_server_tid) {
        return -1;
    }
    if (ticks < 0) {
        return -2;
    }

    char request[16];
    memset(request, 0, 16);
    request[0] = DELAY;
    request[1] = ' ';
    ui2a(ticks, 10, request + 2);

    char response[16];
    int64_t ret = send(clock_server_tid, request, 16, response, 16);

    if (ret < 0) {
        return -1;
    } else {
        return a2ui(response, 10);
    }
}

int64_t delay_until(uint64_t tid, int64_t ticks)
{
    uint64_t clock_server_tid = who_is("clock_server");
    if (tid != clock_server_tid) {
        return -1;
    }

    char request[16];
    memset(request, 0, 16);
    request[0] = DELAY_UNTIL;
    request[1] = ' ';
    ui2a(ticks, 10, request + 2);

    char response[16];
    int64_t ret = send(clock_server_tid, request, 16, response, 16);

    if (ret == 0) {
        return -2;
    } else if (ret < 0) {
        return -1;
    } else {
        return a2ui(response, 10);
    }
}

void k3_clock_server()
{
    register_as("clock_server");

    uintmap_t waiting_tasks = uintmap_new(); // map<tid, time>
    uint32_t cur_ticks = 0;

    char argv[4][32];
    char msg[32];
    for (;;) {
        uint64_t caller_tid;
        int64_t msglen = receive(&caller_tid, msg, 32);
        msg[msglen] = 0;
        int argc = split(msg, argv);

        switch (argv[0][0]) {
        case TIME: {
            ASSERT(argc == 1, "time expects 1 argument");
            uint32_t ticks_passed = timer_get_ticks() - cur_ticks;
            reply_num(caller_tid, ticks_passed);
            break;
        }
        case DELAY: {
            ASSERT(argc == 2, "delay expects 2 arguments");
            uint32_t delay_until = a2ui(argv[1], 10);
            uintmap_set(&waiting_tasks, caller_tid, delay_until);
            break;
        }
        case DELAY_UNTIL: {
            ASSERT(argc == 2, "delay_until expects 2 arguments");
            uint32_t delay_until = a2ui(argv[1], 10);
            if (delay_until < cur_ticks) {
                reply_empty(caller_tid);
                break;
            }
            uintmap_set(&waiting_tasks, caller_tid, delay_until);
            break;
        }
        case TICK: {
            cur_ticks++;
            for (int i = 0; i < waiting_tasks.num_keys; ++i) {
                uint64_t tid = waiting_tasks.keys[i];
                uint64_t delay_until = waiting_tasks.values[i];
                if (delay_until <= cur_ticks) { // <= handles the case of delay(0)
                    reply_num(tid, delay_until);
                    uintmap_remove(&waiting_tasks, tid);
                }
            }
        }
        }
    }
}

#endif
