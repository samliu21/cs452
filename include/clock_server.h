#ifndef _clock_server_h_
#define _clock_server_h_

#include "common.h"

typedef enum {
    TIME = 1,
    DELAY = 2,
    DELAY_UNTIL = 3,
    TICK = 4,
} clock_server_request_t;

int64_t time(uint64_t tid);
int64_t delay(uint64_t tid, int64_t ticks);
int64_t delay_until(uint64_t tid, int64_t ticks);

void clock_server_task();

#endif
