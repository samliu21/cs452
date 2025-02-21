#ifndef _name_server_h_
#define _name_server_h_

#include "common.h"

typedef enum {
    WHO_IS = 1,
    REGISTER_AS = 2,
} name_server_request_t;

#define NAME_SERVER_TID 2

#define TERMINAL_TASK_NAME "terminal_task"
#define MARKLIN_TASK_NAME "marklin_task"
#define CLOCK_TASK_NAME "clock_task"
#define COMMAND_TASK_NAME "command_task"
#define STATE_TASK_NAME "state_task"
#define DISPLAY_STATE_TASK_NAME "display_task"

int64_t register_as(const char* name);
int64_t who_is(const char* name);
void name_server_task();

#endif
