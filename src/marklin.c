#include "marklin.h"
#include "clock_server.h"
#include "name_server.h"
#include "state_server.h"
#include "syscall_func.h"
#include "train.h"
#include "uart_server.h"

void marklin_set_speed(uint64_t train, uint64_t speed)
{
    char buf[3];
    buf[0] = speed + 16;
    buf[1] = train;
    buf[2] = 0;
    puts(MARKLIN, buf);
}

void marklin_reverse(uint64_t train)
{
    char buf[3];
    buf[0] = 0x1f;
    buf[1] = train;
    buf[2] = 0;
    puts(MARKLIN, buf);
}

void marklin_set_switch(uint64_t sw, char d)
{
    char buf[3];
    buf[0] = d == 'S' ? 0x21 : 0x22;
    buf[1] = sw;
    buf[2] = 0;
    puts(MARKLIN, buf);
}

void train_reverse_task()
{
    // get train tid
    uint64_t caller_tid;
    char args[3];
    receive(&caller_tid, args, 10);
    reply_empty(caller_tid);
    int train = args[0];

    // stop train
    train_set_speed(train, 0);
    marklin_set_speed(train, 0);

    // delay
    int64_t ret = delay(350);
    ASSERT(ret >= 0, "delay failed");

    train_set_reverse(train);
    marklin_reverse(train);

    // set train speed
    train_set_speed(train, 10);
    marklin_set_speed(train, 10);

    exit();
}

void deactivate_solenoid_task()
{
    int64_t ret = delay(1000);
    ASSERT(ret >= 0, "delay failed");

    putc(MARKLIN, 0x20);

    exit();
}
