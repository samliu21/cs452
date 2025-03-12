#include "train.h"
#include "clock_server.h"
#include "common.h"
#include "marklin.h"
#include "name_server.h"
#include "state_server.h"
#include "switch.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include "timer.h"
#include "track_data.h"
#include "track_node.h"
#include "train_data.h"
#include "uart_server.h"
#include <stdlib.h>

#define LOOKAHEAD_DISTANCE 1000
#define SENSOR_PREDICTION_WINDOW 300

trainlist_t trainlist_create(train_t* trains)
{
    trainlist_t tlist;
    tlist.trains = trains;
    tlist.size = 0;
    return tlist;
}

void trainlist_add(trainlist_t* tlist, uint64_t id)
{
    tlist->trains[tlist->size].id = id;
    tlist->trains[tlist->size].speed = 0;
    tlist->trains[tlist->size].old_speed = 0;
    tlist->trains[tlist->size].speed_time_begin = 0;
    tlist->trains[tlist->size].last_sensor = -1;
    tlist->trains[tlist->size].stop_node = -1;
    tlist->trains[tlist->size].reverse_direction = 0;
    tlist->trains[tlist->size].cur_node = 0;
    tlist->trains[tlist->size].path = track_path_new();
    track_path_add(&tlist->trains[tlist->size].path, 140, 1e9); // EN9 hardcoded for train 55
    train_data_t train_data = init_train_data_a();
    tlist->trains[tlist->size].acc = 0;
    tlist->trains[tlist->size].acc_start = 0;
    tlist->trains[tlist->size].acc_end = 0;
    tlist->trains[tlist->size].cur_offset = train_data.train_length[id];

    tlist->size++;
}

train_t* trainlist_find(trainlist_t* tlist, uint64_t id)
{
    for (int i = 0; i < tlist->size; ++i) {
        if (tlist->trains[i].id == id) {
            return &tlist->trains[i];
        }
    }
    return NULL;
}

typedef enum {
    SET_TRAIN_SPEED = 1,
    GET_TRAIN_SPEED = 2,
    TRAIN_EXISTS = 3,
    SENSOR_READING = 4,
    GET_TRAIN_TIMES = 5,
    TRAIN_LAST_SENSOR = 6,
    SET_STOP_NODE = 7,
    SET_TRAIN_REVERSE = 8,
    GET_TRAIN_REVERSE = 9,
    SHOULD_UPDATE_TRAIN_STATE = 10,
    GET_CUR_NODE = 11,
    GET_CUR_OFFSET = 12,
    ROUTE_TRAIN = 13,
} train_task_request_t;

void train_set_speed(uint64_t train, uint64_t speed)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[3];
    buf[0] = SET_TRAIN_SPEED;
    buf[1] = speed;
    buf[2] = train;
    int64_t ret = send(train_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

uint64_t train_get_speed(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2], response[2];
    buf[0] = GET_TRAIN_SPEED;
    buf[1] = train;
    int64_t ret = send(train_task_tid, buf, 2, response, 2);
    ASSERT(ret >= 0, "send failed");
    return response[1];
}

int train_exists(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = TRAIN_EXISTS;
    buf[1] = train;
    char response;
    int64_t ret = send(train_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    return response;
}

void train_sensor_reading(track_node* track, char* sensor)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    int node_index = name_to_node_index(track, sensor);
    ASSERTF(node_index >= 0, "invalid sensor name: '%s'", sensor);
    char buf[2];
    buf[0] = SENSOR_READING;
    buf[1] = node_index;
    int64_t ret = send(train_task_tid, buf, 2, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void train_get_times(char* response)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char c = GET_TRAIN_TIMES;
    int64_t ret = send(train_task_tid, &c, 1, response, 256);
    ASSERT(ret >= 0, "send failed");
}

int train_last_sensor(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = TRAIN_LAST_SENSOR;
    buf[1] = train;
    char response;
    int64_t ret = send(train_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    if (ret == 0) {
        return -1;
    }
    return response;
}

void train_set_stop_node(uint64_t train, int node, int time_offset)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[32];
    buf[0] = SET_STOP_NODE;
    buf[1] = train;
    buf[2] = node;
    ui2a(time_offset, 10, buf + 3);
    int64_t ret = send(train_task_tid, buf, 32, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void train_set_reverse(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = SET_TRAIN_REVERSE;
    buf[1] = train;
    int64_t ret = send(train_task_tid, buf, 2, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

int train_get_reverse(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = GET_TRAIN_REVERSE;
    buf[1] = train;
    char response;
    int64_t ret = send(train_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    return response;
}

void train_stop_task()
{
    int64_t ret;
    uint64_t caller_tid;
    char buf[32];

    ret = receive(&caller_tid, buf, 32);
    ASSERT(ret >= 0, "receive failed");
    ret = reply_empty(caller_tid);
    ASSERT(ret >= 0, "reply failed");

    uint64_t train = buf[0];
    uint64_t delay_ms = a2ui(buf + 1, 10);
    ret = delay(delay_ms / 10); // delay is in ms
    ASSERT(ret >= 0, "delay failed");

    marklin_set_speed(train, 0);
    train_set_speed(train, 0);

    syscall_exit();
}

int train_get_cur_node(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = GET_CUR_NODE;
    buf[1] = train;
    char response;
    int64_t ret = send(train_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    return response;
}

int train_get_cur_offset(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = GET_CUR_OFFSET;
    buf[1] = train;
    char response[8];
    int64_t ret = send(train_task_tid, buf, 2, response, 8);
    ASSERT(ret >= 0, "send failed");
    return a2i(response, 10);
}

void train_route(uint64_t train, int dest, int offset)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[8];
    buf[0] = ROUTE_TRAIN;
    buf[1] = train;
    buf[2] = dest;
    i2a(offset, buf + 3);
    int64_t ret = send(train_task_tid, buf, 8, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void should_update_train_state()
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char c = SHOULD_UPDATE_TRAIN_STATE;
    int64_t ret = send(train_task_tid, &c, 1, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void train_model_notifier()
{
    int cur_time = time();
    for (;;) {
        should_update_train_state();

        cur_time += 5;
        delay_until(cur_time);
    }
}

void set_train_speed_handler(train_data_t* train_data, train_t* t, uint64_t speed)
{
    int old_speed = t->speed;
    if (old_speed == 0 && speed > 0) {
        t->acc = train_data->acc_start[t->id][speed];
        t->acc_start = timer_get_ms();
        t->acc_end = timer_get_ms() + train_data->starting_time[t->id][speed];
    } else if (old_speed > 0 && speed == 0) {
        t->acc = train_data->acc_stop[t->id][speed];
        t->acc_start = timer_get_ms();
        t->acc_end = timer_get_ms() + train_data->stopping_time[t->id][speed];
    }
    t->speed = speed;
    t->old_speed = old_speed;
    t->speed_time_begin = timer_get_ms();
}

void train_task()
{
    int64_t ret;

    ret = register_as(TRAIN_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");

    train_t trains[5];
    trainlist_t trainlist = trainlist_create(trains);
    trainlist_add(&trainlist, 55);
    trainlist_add(&trainlist, 77);

    for (int i = 0; i < trainlist.size; ++i) {
        marklin_set_speed(trainlist.trains[i].id, 0);
    }

    track_node track[TRACK_MAX];
#ifdef TRACKA
    init_tracka(track);
#else
    init_trackb(track);
#endif

    train_data_t train_data = init_train_data_a();

    uint64_t caller_tid;
    char buf[32];
    char train_times[64];
    train_times[0] = '\0';
    for (;;) {
        ret = receive(&caller_tid, buf, 32);
        ASSERT(ret >= 0, "receive failed");

        switch (buf[0]) {
        case SET_TRAIN_SPEED: {
            uint64_t speed = buf[1];
            uint64_t train = buf[2];
            train_t* t = trainlist_find(&trainlist, train);
            if (t == NULL) {
                ret = reply_num(caller_tid, 1);
                ASSERT(ret >= 0, "reply failed");
                break;
            }

            set_train_speed_handler(&train_data, t, speed);

            ret = reply_num(caller_tid, 0);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_TRAIN_SPEED: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            char response[2];
            if (t == NULL) {
                response[0] = 1;
            } else {
                response[0] = 0;
                response[1] = t->speed;
            }
            ret = reply(caller_tid, response, 2);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case TRAIN_EXISTS: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ret = reply_char(caller_tid, t == NULL ? 0 : 1);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SENSOR_READING: {
            int node_index = buf[1];

            // for (int i = 0; i < trainlist.size; ++i) {
            //     if (node_index == trainlist.trains[i].stop_node) {
            //         int64_t stop_task_tid = create(1, &train_stop_task);
            //         ASSERT(stop_task_tid >= 0, "create failed");

            //         char time_offset_buf[32];
            //         time_offset_buf[0] = trainlist.trains[i].id;
            //         ui2a(trainlist.trains[i].stop_time_offset, 10, time_offset_buf + 1);
            //         ret = send(stop_task_tid, time_offset_buf, 32, NULL, 0);
            //         ASSERT(ret >= 0, "send failed");

            //         trainlist.trains[i].stop_node = -1;
            //     }
            // }

            // if the train has not received a sensor reading yet, set the reachable sensors from the initial reading
            // otherwise, find the train expecting this sensor reading and update its reachable sensors
            // if (!has_received_initial_sensor) {
            //     int train = -1;
            //     for (int i = 0; i < trainlist.size; ++i) {
            //         if (trainlist.trains[i].speed > 0) {
            //             train = i;
            //             break;
            //         }
            //     }
            //     if (train >= 0) {
            //         has_received_initial_sensor = 1;
            //         trainlist.trains[train].sensors = get_reachable_sensors(track, node_index);
            //         trainlist.trains[train].last_sensor = node_index;
            //     }
            //     goto sensor_reading_end;
            // }

            for (int i = 0; i < trainlist.size; ++i) {
                train_t* train = &trainlist.trains[i];

                int distance_to_sensor = 1e9;
                if (train->path.nodes[train->cur_node] == node_index) {
                    distance_to_sensor = train->cur_offset;
                } else if (train->cur_node + 1 < train->path.path_length && train->path.nodes[train->cur_node + 1] == node_index) {
                    distance_to_sensor = train->cur_offset - train->path.distances[train->cur_node]; // neg number
                }

                if (distance_to_sensor > -SENSOR_PREDICTION_WINDOW && distance_to_sensor < SENSOR_PREDICTION_WINDOW) {
                    sprintf(train_times, "distance delta: %dmm", distance_to_sensor);

                    if (distance_to_sensor < 0) {
                        train->cur_node++;
                    }
                    ASSERT(train->path.nodes[train->cur_node] == node_index, "train isn't at sensor node");
                    train->cur_offset = 0;

                    // train->sensors = get_reachable_sensors(track, node_index);
                    // train->last_sensor = node_index;
                    goto sensor_reading_end;
                }
            }

        sensor_reading_end:
            reply_empty(caller_tid);
            break;
        }
        case GET_TRAIN_TIMES: {
            ret = reply(caller_tid, train_times, strlen(train_times) + 1);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case TRAIN_LAST_SENSOR: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);

            if (t != NULL) {
                char c = t->last_sensor;
                ret = reply(caller_tid, &c, 1);
                ASSERT(ret >= 0, "reply failed");
                break;
            }

            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SET_STOP_NODE: {
            uint64_t train = buf[1];
            int node = buf[2];
            int time_offset = a2ui(buf + 3, 10);
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            t->stop_node = node;
            t->stop_time_offset = time_offset;
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SET_TRAIN_REVERSE: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            t->reverse_direction = !t->reverse_direction;
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_TRAIN_REVERSE: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            ret = reply_char(caller_tid, t->reverse_direction);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SHOULD_UPDATE_TRAIN_STATE: {
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            for (int i = 0; i < trainlist.size; ++i) {
                train_t* t = &trainlist.trains[i];

                int interval_end = timer_get_ms();
                int interval_start = interval_end - 50;

                int acc_start = min(interval_end, max(interval_start, t->acc_start));
                int acc_end = min(interval_end, max(interval_start, t->acc_end));

                int acc_duration = acc_end - acc_start;
                int new_speed_duration = interval_end - acc_end;
                int old_speed_duration = acc_start - interval_start;

                int vi = max(0, (interval_start - t->speed_time_begin) * t->acc);

                int dx = vi * acc_duration + t->acc * acc_duration * acc_duration / 2;

                // mm/s^2 * ms * ms
                // int acc_distance = t->old_speed * acc_duration / 1000 + t->acc * acc_duration * acc_duration / 2000000;
                int new_speed_distance = new_speed_duration * train_data.speed[t->id][t->speed];
                int old_speed_distance = old_speed_duration * train_data.speed[t->id][t->old_speed];
                // int update_distance = (dx + (new_speed_distance + old_speed_distance) * 1000) / 1000000;
                t->total_distance_tiny += dx + (new_speed_distance + old_speed_distance) * 1000;

                int update_distance = (t->total_distance_tiny - t->total_distance_accumulated) / 1000000;
                t->total_distance_accumulated += update_distance * 1000000;

                t->cur_offset += update_distance;
                if (t->cur_offset >= t->path.distances[t->cur_node]) {
                    t->cur_offset -= t->path.distances[t->cur_node++];

                    // if (t->path.nodes[t->cur_node] == 57) {
                    //     marklin_set_speed(55, 15);
                    //     printf(CONSOLE, "end time: %d\r\n", timer_get_ms());
                    // }
                }

                if (t->path.nodes[t->cur_node] == t->stop_node && t->cur_offset >= t->stop_distance_offset) {
                    marklin_set_speed(t->id, 0);

                    set_train_speed_handler(&train_data, t, 0);

                    t->stop_node = -1;
                }

                // int distance_ahead = 0;
                // int cur_node_index = t->cur_node;
                // while (cur_node_index < t->path.path_length - 1 && distance_ahead < LOOKAHEAD_DISTANCE) {
                //     track_node* cur_node = &track[t->path.nodes[cur_node_index]];
                //     track_node* nxt_node = &track[t->path.nodes[cur_node_index + 1]];
                //     distance_ahead += t->path.nodes[cur_node_index];

                //     for (int i = 0; i < 2; ++i) {
                //         if (cur_node->enters_seg[i] >= 0 && cur_node->edge[i].dest == nxt_node) {
                //             int reserver = state_is_reserved(cur_node->enters_seg[i]);
                //             if (reserver && reserver != t->id) {
                //                 printf(CONSOLE, "collision detected: %d %d", t->id, reserver);
                //             } else {
                //                 state_reserve_segment(cur_node->enters_seg[i], t->id);
                //             }
                //         }
                //     }

                //     cur_node_index++;
                // }
            }
            break;
        }
        case GET_CUR_NODE: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            ret = reply_char(caller_tid, t->path.nodes[t->cur_node]);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_CUR_OFFSET: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            char response[8];
            i2a(t->cur_offset, response);
            ret = reply(caller_tid, response, 8);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case ROUTE_TRAIN: {
            uint64_t train = buf[1];
            int dest = buf[2];
            int offset = a2i(buf + 3, 10);
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            t->path = get_shortest_path(track, t, dest, offset);
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");

            for (int i = t->path.path_length - 2; i >= 0; --i) {
                track_node node = track[t->path.nodes[i]];
                if (node.type == NODE_BRANCH) {
                    char switch_type = (get_node_index(track, node.edge[DIR_STRAIGHT].dest) == t->path.nodes[i + 1]) ? S : C;

                    create_switch_task(node.num, switch_type);
                }
            }
            int64_t ret = create(1, &deactivate_solenoid_task);
            ASSERT(ret >= 0, "create failed");

            printf(CONSOLE, "stop node: %d, stop time offset %d\r\n", t->path.stop_node, t->path.stop_time_offset);
            t->stop_node = t->path.stop_node;
            t->stop_time_offset = t->path.stop_time_offset;
            t->stop_distance_offset = t->path.stop_distance_offset;

            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }
}
