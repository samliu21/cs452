#include "marklin.h"
#include "clock_server.h"
#include "name_server.h"
#include "state_server.h"
#include "syscall_func.h"
#include "train.h"
#include "uart_server.h"

void marklin_set_speed(uint64_t marklin_task_tid, uint64_t train, uint64_t speed)
{
    char buf[3];
    buf[0] = speed + 16;
    buf[1] = train;
    buf[2] = 0;
    puts(marklin_task_tid, MARKLIN, buf);
}

void marklin_set_switch(uint64_t marklin_task_tid, uint64_t sw, char d)
{
    char buf[3];
    buf[0] = d == 'S' ? 0x21 : 0x22;
    buf[1] = sw;
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
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    // get train tid
    uint64_t caller_tid;
    char train;
    receive(&caller_tid, &train, 1);
    reply_empty(caller_tid);

    // delay
    int64_t ret = delay(clock_task_tid, 350);
    ASSERT(ret >= 0, "delay failed");

    // reverse train
    char sendbuf[3];
    sendbuf[0] = 0x1f;
    sendbuf[1] = train;
    sendbuf[2] = 0;
    puts(marklin_task_tid, MARKLIN, sendbuf);

    // set train speed
    uint64_t speed = state_get_speed(train_task_tid, train);
    marklin_set_speed(marklin_task_tid, train, speed);

    exit();
}

void deactivate_solenoid_task()
{
    int64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    ASSERT(clock_task_tid >= 0, "who_is failed");
    int64_t marklin_task_tid = who_is(MARKLIN_TASK_NAME);
    ASSERT(marklin_task_tid >= 0, "who_is failed");

    int64_t ret = delay(clock_task_tid, 1000);
    ASSERT(ret >= 0, "delay failed");

    putc(marklin_task_tid, MARKLIN, 0x20);

    exit();
}
