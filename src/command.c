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

    // A3 C13 E7 D7 D9 E12 C6 B15
    int loop_sensors[NUM_LOOP_SENSORS] = { 2, 44, 70, 54, 56, 75, 37, 30 };
    int loop_switches[NUM_LOOP_SWITCHES] = { 14, 8, 7, 18 };
    switchstate_t loop_switch_states[NUM_LOOP_SWITCHES] = { S, S, C, C };

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

            marklin_set_speed(train, 0);

            int64_t reverse_task_id = create(1, &train_reverse_task);
            int64_t ret = send(reverse_task_id, (char*)&train, 1, NULL, 0);
            ASSERT(ret >= 0, "send failed");

            train_set_reverse(train);

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
            ASSERT(ret >= 0, "create failed");

            result.type = COMMAND_SUCCESS;
        }
        /*
        else if (strcmp(command_type, "go") == 0) {
            if (argc != 4) {
                result.type = COMMAND_FAIL;
                error_message = "go command expects 3 arguments";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            int dest = name_to_node_index(track, args[2]);
            int node_offset = a2i(args[3], 10);

            if (!train_exists(train)) {
                result.type = COMMAND_FAIL;
                error_message = "train does not exist";
                goto end;
            }
            uint64_t speed_level = train_get_speed(train);
            if (speed_level != 6 && speed_level != 10) {
                result.type = COMMAND_FAIL;
                error_message = "train must be at speed 6 (LOW) or 10 (HIGH)";
                goto end;
            }
            if (dest == -1) {
                result.type = COMMAND_FAIL;
                error_message = "invalid destination node";
                goto end;
            }

            int last_sensor = train_last_sensor(train);

            if (last_sensor == -1) {
                result.type = COMMAND_FAIL;
                error_message = "train is not ready to go";
                goto end;
            }

            int src = state_next_sensor(last_sensor);
            if (last_sensor == -1) {
                result.type = COMMAND_FAIL;
                error_message = "no next sensor";
                goto end;
            }

            track_path_t path = get_shortest_path(track, src, dest, node_offset, train);
            for (int i = path.path_length - 2; i >= 0; --i) {
                track_node node = track[path.nodes[i]];
                if (node.type == NODE_BRANCH) {
                    char switch_type = (get_node_index(track, node.edge[DIR_STRAIGHT].dest) == path.nodes[i + 1]) ? S : C;

                    create_switch_task(node.num, switch_type);
                }
            }
            int64_t ret = create(1, &deactivate_solenoid_task);
            ASSERT(ret >= 0, "create failed");

            // printf(CONSOLE, "stop node: %d, time offset: %d\r\n", path.stop_node, path.stop_time_offset);
            train_set_stop_node(train, path.stop_node, path.stop_time_offset);

            result.type = COMMAND_SUCCESS;
        }
        */

        else if (strcmp(command_type, "route") == 0) { // route <train> <dest> <offset>
            if (argc != 4) {
                result.type = COMMAND_FAIL;
                error_message = "route command expects 3 arguments";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            int dest = name_to_node_index(track, args[2]);
            int node_offset = a2i(args[3], 10);

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
            printf(CONSOLE, "start time: %d\r\n", timer_get_ms());

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "lp") == 0) {
            if (argc != 3) {
                result.type = COMMAND_FAIL;
                error_message = "lp command expects 2 arguments";
                goto end;
            }
            uint64_t train = a2ui(args[1], 10);
            uint64_t speed = a2ui(args[2], 10);
            if (!train_exists(train)) {
                result.type = COMMAND_FAIL;
                error_message = "train does not exist";
                goto end;
            } else if (train_get_speed(train) != 0) {
                result.type = COMMAND_FAIL;
                error_message = "train must be stopped first";
                goto end;
            } else if (speed > 15) {
                result.type = COMMAND_FAIL;
                error_message = "speed must be between 0 and 14";
                goto end;
            }

            int last_sensor = train_last_sensor(train);

            if (last_sensor == -1) {
                result.type = COMMAND_FAIL;
                error_message = "train is not ready to go";
                goto end;
            }

            int rv = 0;
            int src = state_next_sensor(last_sensor);
            if (src == -1) {
                src = get_node_index(track, track[last_sensor].reverse);
                rv = 1;
            }

            track_path_t path = get_closest_node(track, src, loop_sensors, NUM_LOOP_SENSORS);

            if (path.path_length == 0) {
                src = get_node_index(track, track[src].reverse);
                path = get_closest_node(track, src, loop_sensors, NUM_LOOP_SENSORS);
                rv = 1;
                if (path.path_length == 0) {
                    result.type = COMMAND_FAIL;
                    error_message = "could not find loop";
                    goto end;
                }
            }

            for (int i = 0; i < path.path_length - 1; ++i) {
                track_node node = track[path.nodes[i]];
                if (node.type == NODE_BRANCH) {
                    char switch_type = (get_node_index(track, node.edge[DIR_STRAIGHT].dest) == path.nodes[i + 1]) ? S : C;

                    create_switch_task(node.num, switch_type);
                }
            }

            for (int i = 0; i < NUM_LOOP_SWITCHES; ++i) {
                create_switch_task(loop_switches[i], loop_switch_states[i]);
            }
            int64_t ret = create(1, &deactivate_solenoid_task);
            ASSERT(ret >= 0, "create failed");

            if (rv == 1) {
                train_set_reverse(train);
                marklin_reverse(train);
            }

            train_set_speed(train, speed);

            marklin_set_speed(train, speed);

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
