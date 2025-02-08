#ifndef _state_server_h_
#define _state_server_h_

#include "name_server.h"
#include "rpi.h"
#include "syscall_func.h"
#include "train.h"

typedef enum {
    SET_TRAIN_SPEED = 0,
    GET_TRAIN_SPEED = 1,
} state_server_request_t;

void state_task()
{
    int64_t res = register_as(STATE_SERVER_NAME);
    ASSERT(res >= 0, "register_as failed");

    train_t trains[5];
    trainlist_t trainlist = train_createlist(trains);
    train_add(&trainlist, 55);

    uint64_t caller_tid;
    char buf[2];
    for (;;) {
        receive(&caller_tid, buf, 2);
        uint64_t train = buf[0];
        uint64_t speed = buf[1];

        train_t* t = train_find(&trainlist, train);
        if (t == NULL) {
            train_add(&trainlist, train);
            t = train_find(&trainlist, train);
        }
        t->speed = speed;

        reply_empty(caller_tid);
    }

    exit();
}

#endif
