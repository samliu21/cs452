#include "command.h"
#include "marklin.h"
#include "name_server.h"
#include "rpi.h"
#include "state_server.h"
#include "switch.h"
#include "syscall_func.h"
#include "timer.h"
#include "track_algo.h"
#include "track_data.h"
#include "train.h"
#include "uart_server.h"

#define NUM_LOOP_SENSORS 8
#define NUM_LOOP_SWITCHES 4

void command_task()
{
    int64_t ret = register_as(COMMAND_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");

    track_node track[TRACK_MAX];
#ifdef TRACKA
    init_tracka(track);
#else
    init_trackb(track);
#endif

    char command[32];
    uint64_t caller_tid;
    char* error_message = NULL;

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
                error_message = "tr command expects 2 arguments";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            uint64_t speed = a2ui(args[2], 10);
            if (!train_exists(train)) {
                result.type = COMMAND_FAIL;
                error_message = "train does not exist";
                goto end;
            } else if (speed > 15) {
                result.type = COMMAND_FAIL;
                error_message = "speed must be between 0 and 14";
                goto end;
            }

            // update state
            train_set_speed(train, speed);

            // set speed on marklin
            marklin_set_speed(train, speed);

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "rv") == 0) {
            if (argc != 2) {
                result.type = COMMAND_FAIL;
                error_message = "rv command expects 1 argument";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            if (!train_exists(train)) {
                result.type = COMMAND_FAIL;
                error_message = "train does not exist";
                goto end;
            }

            int64_t reverse_task_id = create(1, &train_reverse_task);
            int64_t ret = send(reverse_task_id, (char*)&train, 1, NULL, 0);
            ASSERT(ret >= 0, "reverse task startup send failed");

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "sw") == 0) {
            if (argc != 3) {
                result.type = COMMAND_FAIL;
                error_message = "sw command expects 2 arguments";
                goto end;
            }
            unsigned int num = a2ui(args[1], 10);
            char d = args[2][0];

            if (!state_switch_exists(num)) {
                result.type = COMMAND_FAIL;
                error_message = "switch does not exist";
                goto end;
            }

            if (d != S && d != C) {
                result.type = COMMAND_FAIL;
                error_message = "switch direction must be S or C";
                goto end;
            }

            create_switch_task(num, d);
            int64_t ret = create(1, &deactivate_solenoid_task);
            ASSERTF(ret >= 0, "create failed: %d", ret);

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "route") == 0) { // route <train> <dest> <offset>
            if (argc < 3 || argc > 4) {
                result.type = COMMAND_FAIL;
                error_message = "route command expects 3-4 arguments";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            int dest = name_to_node_index(track, args[2]);

            int node_offset = (argc == 4) ? a2i(args[3], 10) : 0;

            if (!train_exists(train)) {
                result.type = COMMAND_FAIL;
                error_message = "train does not exist";
                goto end;
            }
            uint64_t speed_level = train_get_speed(train);
            if (speed_level != 0) {
                result.type = COMMAND_FAIL;
                error_message = "train must be at a stop";
                goto end;
            }
            if (dest == -1) {
                result.type = COMMAND_FAIL;
                error_message = "invalid destination node";
                goto end;
            }

            // set speed for routing purposes
            train_set_speed(train, 10);

            train_route(train, dest, node_offset);

            marklin_set_speed(train, 10);

            // printf(CONSOLE, "start time: %d\r\n", timer_get_ms());

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "routemany") == 0) { // route <train> <dest> <offset>
            if (argc < 3 || argc % 2 != 1) {
                result.type = COMMAND_FAIL;
                error_message = "routemany command expects sets of 2 arguments";
                
                goto end;
            }
            for (int i = 0; i < ((argc - 1) / 2); ++i) {
                uint64_t train = a2ui(args[(i * 2) + 1], 10);
                int dest = name_to_node_index(track, args[(i * 2) + 2]);

                if (!train_exists(train)) {
                    result.type = COMMAND_FAIL;
                    error_message = "train does not exist";
                    goto end;
                }
                uint64_t speed_level = train_get_speed(train);
                if (speed_level != 0) {
                    result.type = COMMAND_FAIL;
                    error_message = "train must be at a stop";
                    goto end;
                }
                if (dest == -1) {
                    result.type = COMMAND_FAIL;
                    error_message = "invalid destination node";
                    goto end;
                }

                // set speed for routing purposes
                train_set_speed(train, 10);

                train_route(train, dest, 0);

                marklin_set_speed(train, 10);
            }

            // printf(CONSOLE, "start time: %d\r\n", timer_get_ms());

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "rs") == 0) { // rs <segment>
            if (argc != 2) {
                result.type = COMMAND_FAIL;
                error_message = "reserve command expects 1 argument";
                goto end;
            }
            uint64_t segment = a2ui(args[1], 10);

            int reserver = state_is_reserved(segment);
            if (reserver) {
                state_release_segment(segment, reserver);
            } else {
                state_reserve_segment(segment, '\255');
            }

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "fb") == 0) { // fb <segment>
            if (argc != 2) {
                result.type = COMMAND_FAIL;
                error_message = "forbid command expects 1 argument";
                goto end;
            }
            uint64_t segment = a2ui(args[1], 10);
          
            state_forbid_segment(segment);

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "q") == 0) {
            result.type = COMMAND_QUIT;
        }

        else {
            result.type = COMMAND_FAIL;
            char msg[128];
            sprintf(msg, "invalid command: %s", command);
            strcpy(result.error_message, msg); // copy directly into result.error_message
            error_message = NULL;
        }

    end:
        if (error_message != NULL) {
            strcpy(result.error_message, error_message);
        }
        reply(caller_tid, (char*)&result, sizeof(command_result_t));
    }
}
