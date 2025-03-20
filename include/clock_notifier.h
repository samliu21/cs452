#ifndef _clock_notifier_h_
#define _clock_notifier_h_

#include "clock_server.h"
#include "name_server.h"
#include "syscall_asm.h"
#include "timer.h"

void clock_notifier_task()
{
    int64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    ASSERT(clock_task_tid >= 0, "who_is failed");
    char tick_msg[2];
    tick_msg[0] = TICK;
    tick_msg[1] = 0;
    for (;;) {
        await_event(EVENT_TICK);
        int ret = send(clock_task_tid, tick_msg, 2, NULL, 0);
        ASSERT(ret >= 0, "clock notifier send failed");
    }

    exit();
}

#endif
