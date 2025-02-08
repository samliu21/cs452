#include "marklin.h"
#include "clock_server.h"
#include "name_server.h"
#include "state_server.h"
#include "syscall_func.h"
#include "uart_server.h"

void marklin_set_speed(uint64_t marklin_task_tid, uint64_t train, uint64_t speed)
{
    char buf[3];
    buf[0] = speed + 16;
    buf[1] = train;
    buf[2] = 0;
    puts(marklin_task_tid, MARKLIN, buf);
}

void train_reverse_task()
{
    int64_t marklin_task_tid = who_is(MARKLIN_TASK_NAME);
    ASSERT(marklin_task_tid >= 0, "who_is failed");
    int64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    ASSERT(clock_task_tid >= 0, "who_is failed");
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    // get train tid
    uint64_t caller_tid;
    char train;
    receive(&caller_tid, &train, 1);
    reply_empty(caller_tid);

    // delay
    int64_t res = delay(clock_task_tid, 350);
    ASSERT(res >= 0, "delay failed");

    // reverse train
    char sendbuf[3];
    sendbuf[0] = 0x1f;
    sendbuf[1] = train;
    sendbuf[2] = 0;
    puts(marklin_task_tid, MARKLIN, sendbuf);

    // set train speed
    uint64_t speed = state_get_speed(state_task_tid, train);
    marklin_set_speed(marklin_task_tid, train, speed);

    exit();
}
