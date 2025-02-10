#include "clock_server.h"
#include "name_server.h"
#include "queue.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include "timer.h"
#include "uintmap.h"

int64_t time(uint64_t tid)
{
    uint64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    if (tid != clock_task_tid) {
        return -1;
    }
    char request[2];
    request[0] = TIME;
    request[1] = 0;

    char response[16];
    int64_t ret = send(clock_task_tid, request, 2, response, 16);

    if (ret < 0) {
        return -1;
    } else {
        return a2ui(response, 10);
    }
}

int64_t delay(uint64_t tid, int64_t ticks)
{
    uint64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    if (tid != clock_task_tid) {
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
    int64_t ret = send(clock_task_tid, request, 16, response, 16);

    if (ret < 0) {
        return -1;
    } else {
        return a2ui(response, 10);
    }
}

int64_t delay_until(uint64_t tid, int64_t ticks)
{
    uint64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    if (tid != clock_task_tid) {
        return -1;
    }

    char request[16];
    memset(request, 0, 16);
    request[0] = DELAY_UNTIL;
    request[1] = ' ';
    ui2a(ticks, 10, request + 2);

    char response[16];
    int64_t ret = send(clock_task_tid, request, 16, response, 16);

    if (ret == 0) {
        return -2;
    } else if (ret < 0) {
        return -1;
    } else {
        return a2ui(response, 10);
    }
}

void clock_server_task()
{
    register_as(CLOCK_TASK_NAME);

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
            reply_num(caller_tid, cur_ticks);
            break;
        }
        case DELAY: {
            ASSERT(argc == 2, "delay expects 2 arguments");
            uint32_t delay_until = cur_ticks + a2ui(argv[1], 10);
            if (cur_ticks == delay_until) {
                reply_num(caller_tid, cur_ticks);
                break;
            }
            // uart_printf(CONSOLE, "delay_until: %d\n", delay_until);
            uintmap_set(&waiting_tasks, caller_tid, delay_until);
            break;
        }
        case DELAY_UNTIL: {
            ASSERT(argc == 2, "delay_until expects 2 arguments");
            uint32_t delay_until = a2ui(argv[1], 10);
            if (delay_until < cur_ticks) {
                reply_empty(caller_tid);
                break;
            } else if (cur_ticks == delay_until) {
                reply_num(caller_tid, cur_ticks);
                break;
            }
            uintmap_set(&waiting_tasks, caller_tid, delay_until);
            break;
        }
        case TICK: {
            ASSERT(caller_tid == 4, "tick not called by clock notifier");
            cur_ticks++;

            for (int i = waiting_tasks.num_keys - 1; i >= 0; --i) {
                uint64_t tid = waiting_tasks.keys[i];
                uint64_t delay_until = waiting_tasks.values[i];
                ASSERT(delay_until >= cur_ticks, "delay_until < cur_ticks");
                if (delay_until == cur_ticks) {
                    reply_num(tid, delay_until);
                    uintmap_remove(&waiting_tasks, tid);
                }
            }
            reply_empty(caller_tid);
        }
        }
    }
}
