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

void state_task();

void state_set_speed(uint64_t state_task_tid, uint64_t train, uint64_t speed);
uint64_t state_get_speed(uint64_t state_task_tid, uint64_t train);

#endif
