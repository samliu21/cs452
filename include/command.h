#ifndef _command_h_
#define _command_h_

typedef enum {
    COMMAND_QUIT,
    COMMAND_SUCCESS,
    COMMAND_FAIL,
} command_type_t;

typedef enum {
    UP = 'u',
    DOWN = 'd',
    RIGHT = 'l',
    LEFT = 'r',
} arrow_key_t;

typedef struct command_result_t {
    command_type_t type;
    char error_message[128];
} command_result_t;

void player_actable_notifier();
void command_task();

#endif
