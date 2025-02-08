#include "command.h"
#include "name_server.h"
#include "rpi.h"
#include "syscall_func.h"
#include "uart_server.h"

void k4_command_server()
{
    int64_t ret = register_as(COMMAND_SERVER_NAME);
    ASSERT(ret >= 0, "register_as failed");
    int64_t marklin_server_id = who_is(MARKLIN_SERVER_NAME);
    ASSERT(marklin_server_id >= 0, "who_is failed");

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

			// TODO: should this block? should we send these chars to another server?
            putc(marklin_server_id, MARKLIN, 16 + speed);
			putc(marklin_server_id, MARKLIN, 55);

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
