#include "command.h"
#include "marklin.h"
#include "name_server.h"
#include "rpi.h"
#include "state_server.h"
#include "switch.h"
#include "syscall_func.h"
#include "uart_server.h"

void train_reverse_task();

void command_task()
{
    int64_t ret = register_as(COMMAND_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");
    int64_t marklin_task_tid = who_is(MARKLIN_TASK_NAME);
    ASSERT(marklin_task_tid >= 0, "who_is failed");

    char command[32];
    uint64_t caller_tid;

    for (;;) {
        int64_t ret = receive(&caller_tid, command, 32);
        ASSERT(ret >= 0, "receive failed");

        char args[4][32];
        memset(args[0], 0, 128);

        int argc = split(command, args);
        char* command_type = args[0];
        command_result_t result;
        if (strcmp(command_type, "tr") == 0) {
            if (argc != 3) {
                result.type = COMMAND_FAIL;
                result.error_message = "tr command expects 2 arguments";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            uint64_t speed = a2ui(args[2], 10);
            if (!state_train_exists(state_task_tid, train)) {
                result.type = COMMAND_FAIL;
                result.error_message = "train does not exist";
                goto end;
            } else if (speed > 15) {
                result.type = COMMAND_FAIL;
                result.error_message = "speed must be between 0 and 14";
                goto end;
            }

            // update state
            state_set_speed(state_task_tid, train, speed);

            // set speed on marklin
            marklin_set_speed(marklin_task_tid, train, speed);

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "rv") == 0) {
            if (argc != 2) {
                result.type = COMMAND_FAIL;
                result.error_message = "rv command expects 1 argument";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            if (!state_train_exists(state_task_tid, train)) {
                result.type = COMMAND_FAIL;
                result.error_message = "train does not exist";
                goto end;
            }

            marklin_set_speed(marklin_task_tid, train, 0);

            int64_t reverse_task_id = create(1, &train_reverse_task);
            int64_t ret = send(reverse_task_id, (char*)&train, 1, NULL, 0);
            ASSERT(ret >= 0, "send failed");

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "sw") == 0) {
            if (argc != 3) {
                result.type = COMMAND_FAIL;
                result.error_message = "sw command expects 2 arguments";
                goto end;
            }
            unsigned int num = a2ui(args[1], 10);
            char d = args[2][0];

            if (!state_switch_exists(state_task_tid, num)) {
                result.type = COMMAND_FAIL;
                result.error_message = "switch does not exist";
                goto end;
            }

            if (d != S && d != C) {
                result.type = COMMAND_FAIL;
                result.error_message = "switch direction must be S or C";
                goto end;
            }

            state_set_switch(state_task_tid, num, d);

            marklin_set_switch(marklin_task_tid, num, d);
            int64_t ret = create(1, &deactivate_solenoid_task);
            ASSERT(ret >= 0, "create failed");

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "q") == 0) {
            result.type = COMMAND_QUIT;
        }

        else {
            result.type = COMMAND_FAIL;
            result.error_message = "invalid command";
        }

    end:
        reply(caller_tid, (char*)&result, sizeof(command_result_t));
    }
}
