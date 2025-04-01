#include "command.h"
#include "clock_server.h"
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

void player_actable_notifier()
{
    int64_t command_task_tid = who_is(COMMAND_TASK_NAME);
    for (;;) {
        char command[] = "player_actable";
        send(command_task_tid, command, 15, NULL, 0);
        delay(400);
    }
}

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

    int forbidden_dests[NUM_FORBIDDEN_DESTS];
    init_forbidden_destsa(forbidden_dests);

    char switch_left_straight[19];
    init_switch_left_straighta(switch_left_straight);

    char command[32];
    uint64_t caller_tid;
    char* error_message = NULL;

    int player_actable = 0;
    uint64_t notifier_tid = 0;

    for (;;) {
        int64_t ret = receive(&caller_tid, command, 32);
        ASSERT(ret >= 0, "receive failed");

        if (strcmp(command, "player_actable") == 0) {
            player_actable = 1;
            notifier_tid = caller_tid;
            continue;
        }

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
            int invalid_dest = (dest == -1);
            for (int i = 0; i < NUM_FORBIDDEN_DESTS; ++i) {
                if (forbidden_dests[i] == dest) {
                    invalid_dest = 1;
                }
            }
            if (invalid_dest) {
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
                int invalid_dest = (dest == -1);
                for (int i = 0; i < NUM_FORBIDDEN_DESTS; ++i) {
                    if (forbidden_dests[i] == dest) {
                        invalid_dest = 1;
                    }
                }
                if (invalid_dest) {
                    result.type = COMMAND_FAIL;
                    error_message = "invalid destination node";
                    goto end;
                }

                // set speed for routing purposes
                train_set_speed(train, 10);

                train_route(train, dest, 0);

                marklin_set_speed(train, 10);
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

        else if (strcmp(command_type, "rr") == 0) { // fb <segment>
            if (argc < 2) {
                result.type = COMMAND_FAIL;
                error_message = "random reroute command expects at least 1 argument";
                goto end;
            }

            for (int i = 0; i < argc - 1; ++i) {
                uint64_t train = a2ui(args[i + 1], 10);

                if (!train_exists(train)) {
                    result.type = COMMAND_FAIL;
                    error_message = "train does not exist";
                    goto end;
                }

                train_random_reroute(train);

                result.type = COMMAND_SUCCESS;
            }
        }

        else if (strcmp(command_type, "player") == 0) {
            if (argc != 2) {
                result.type = COMMAND_FAIL;
                error_message = "player command expects 1 argument";
                goto end;
            }

            uint64_t train = a2ui(args[1], 10);
            if (!train_exists(train)) {
                result.type = COMMAND_FAIL;
                error_message = "train does not exist";
                goto end;
            }

            train_set_player(train);

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "q") == 0) {
            result.type = COMMAND_QUIT;
        }

        else if (strcmp(command_type, "pc") == 0) {
            int should_disable_user_input = train_should_disable_user_input();
            if (should_disable_user_input) {
                log("Not yet ready to act! User input is temporarily disabled");
            } else {
                switch (args[1][0]) {
                case UP: {
                    if (player_actable) {
                        int train = train_get_player();
                        if (train_get_speed(train) == 0) {
                            if (track[train_get_cur_node(train)].type != NODE_EXIT) {

                                train_set_speed(train, 10);
                                marklin_set_speed(train, 10);
                                player_actable = 0;
                                reply_empty(notifier_tid);
                            } else {
                                log("Player is at exit!\r\n");
                            }
                        } else {
                            log("Player is already moving!\r\n");
                        }
                    } else {
                        log("Not yet ready to act! Still processing previous command.\r\n");
                    }
                    break;
                }
                case LEFT:
                case RIGHT: {
                    int train = train_get_player();
                    int next_switch = train_get_next_switch(train);
                    if (next_switch) {
                        if (next_switch == 153 || next_switch == 154) {
                            int switch_num = (args[1][0] == LEFT) ? 154 : 153;
                            create_switch_task(switch_num, C);
                        } else if (next_switch == 155 || next_switch == 156) {
                            int switch_num = (args[1][0] == LEFT) ? 156 : 155;
                            create_switch_task(switch_num, C);
                        } else {
                            int switch_dir = ((args[1][0] == LEFT) ^ switch_left_straight[next_switch]) ? C : S;
                            log("switched switch %d to %s\r\n", next_switch, (switch_dir == C) ? "C" : "S");
                            create_switch_task(next_switch, switch_dir);
                        }
                    } else {
                        log("no switch in front of train");
                    }
                    break;
                }
                case DOWN: {
                    if (player_actable) {
                        int train = train_get_player();
                        if (train_get_speed(train) > 0) {
                            train_set_speed(train, 0);
                            marklin_set_speed(train, 0);
                            player_actable = 0;
                            reply_empty(notifier_tid);
                        } else {
                            if (track[train_get_cur_node(train)].type != NODE_ENTER) {
                                train_set_reverse(train);
                                marklin_reverse(train);
                                train_set_speed(train, 10);
                                marklin_set_speed(train, 10);
                                player_actable = 0;
                                reply_empty(notifier_tid);
                            } else {
                                log("Player is at enter!\r\n");
                            }
                        }
                    } else {
                        log("Not yet ready to act!\r\n");
                    }
                    break;
                }
                }
            }

            result.type = COMMAND_SUCCESS;
        }

        else if (strcmp(command_type, "race") == 0) {
            char racing_trains[8];
            for (int i = 1; i < argc; ++i) {
                racing_trains[i - 1] = a2i(args[i], 10);
            }
            racing_trains[argc - 1] = 0;
            train_start_race(racing_trains);
            result.type = COMMAND_SUCCESS;
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
