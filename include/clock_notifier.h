#ifndef _clock_notifier_h_
#define _clock_notifier_h_

#include "clock_server.h"
#include "name_server.h"
#include "syscall_asm.h"
#include "timer.h"

void k3_clock_notifier()
{
    uint64_t clock_server_tid = who_is(CLOCK_SERVER_NAME);
    char tick_msg[2];
    tick_msg[0] = TICK;
    tick_msg[1] = 0;
    for (;;) {
        await_event(EVENT_TICK);
        int ret = send(clock_server_tid, tick_msg, 2, NULL, 0);
        ASSERT(ret >= 0, "send failed");
    }

    exit();
}

#endif
