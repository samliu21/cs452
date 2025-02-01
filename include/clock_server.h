#ifndef _clock_server_h_
#define _clock_server_h_

#include "common.h"
#include "queue.h"
#include "syscall_asm.h"

typedef enum {
    TIME = 'T',
    DELAY = 'D',
    DELAY_UNTIL = 'U',
} clock_server_type_t;

void k3_clock_server()
{
    queue_t tasks_waiting_for_event = queue_new();

    char argv[4][32];
    char msg[32];
    for (;;) {
        uint64_t caller_tid;
        int64_t msglen = receive(&caller_tid, msg, 32);
        msg[msglen] = 0;
        int argc = split(msg, argv);
        switch (argv[0][0]) {
        case TIME: {
        }
        }
    }
}

#endif
