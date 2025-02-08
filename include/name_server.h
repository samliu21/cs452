#ifndef _name_server_h_
#define _name_server_h_

#include "common.h"

typedef enum {
    WHO_IS = 'W',
    REGISTER_AS = 'R',
} name_server_request_t;

#define NAME_SERVER_TID 2

#define TERMINAL_SERVER_NAME "uart_terminal_server"
#define MARKLIN_SERVER_NAME "uart_marklin_server"
#define CLOCK_SERVER_NAME "clock_server"

int64_t register_as(const char* name);
int64_t who_is(const char* name);
void k2_name_server();

#endif
