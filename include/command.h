#ifndef _command_h_
#define _command_h_

typedef enum {
    COMMAND_QUIT,
    COMMAND_SUCCESS,
    COMMAND_FAIL,
} command_type_t;

typedef struct command_result_t {
    command_type_t type;
    char* error_message;
} command_result_t;

void command_task();

#endif
