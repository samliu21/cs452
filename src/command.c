#include "command.h"
#include "name_server.h"
#include "rpi.h"
#include "syscall_func.h"
#include "uart_server.h"

void train_reverse_task();

void command_task()
{
    int64_t ret = register_as(COMMAND_SERVER_NAME);
    ASSERT(ret >= 0, "register_as failed");
    int64_t train_speed_server_id = who_is(TRAIN_SPEED_SERVER_NAME);
    ASSERT(train_speed_server_id >= 0, "who_is failed");

    char command[32];
    uint64_t caller_tid;

    for (;;) {
        int64_t ret = receive(&caller_tid, command, 32);
        ASSERT(ret >= 0, "receive failed");

        char args[4][32];
        char* command_type = args[0];
        int argc = split(command, args);
        command_result_t result;
        if (strcmp(command_type, "tr") == 0) {
            if (argc != 3) {
                result.type = COMMAND_FAIL;
                result.error_message = "tr command expects 2 arguments";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            uint64_t speed = a2ui(args[2], 10);
            if (train != 55) {
                result.type = COMMAND_FAIL;
                result.error_message = "train does not exist";
                goto end;
            } else if (speed > 15) {
                result.type = COMMAND_FAIL;
                result.error_message = "speed must be between 0 and 14";
                goto end;
            }
            // marklin_set_train_speed(tlist, q, train, speed);

            char buf[2];
            buf[0] = train;
            buf[1] = speed;
            send(train_speed_server_id, buf, 2, NULL, 0);

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "rv") == 0) {
            if (argc != 2) {
                result.type = COMMAND_FAIL;
                result.error_message = "rv command expects 1 argument";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            if (train != 55) {
                result.type = COMMAND_FAIL;
                result.error_message = "train does not exist";
                goto end;
            }
            // marklin_set_train_speed(tlist, q, train, 0);

            // set speed to 0
            char buf[2];
            buf[0] = train;
            buf[1] = 0;
            send(train_speed_server_id, buf, 2, NULL, 0);

            int64_t reverse_task_id = create(1, &train_reverse_task);
            send(reverse_task_id, buf, 1, NULL, 0);

            result.type = COMMAND_SUCCESS;
        }

        else {
            result.type = COMMAND_FAIL;
            result.error_message = "invalid command";
        }

    end:
        reply(caller_tid, (char*)&result, sizeof(command_result_t));
    }
}
