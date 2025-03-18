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
#include "track_algo.h"
#include "track_data.h"
#include "track_node.h"
#include "train_data.h"
#include "uart_server.h"
#include <stdlib.h>

#define RESERVATION_LOOKAHEAD_DISTANCE 1000
#define SENSOR_PREDICTION_WINDOW 200

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
    tlist->trains[tlist->size].inst_speed = 0;
    tlist->trains[tlist->size].speed_time_begin = 0;
    tlist->trains[tlist->size].last_sensor = -1;
    tlist->trains[tlist->size].stop_node = -1;
    tlist->trains[tlist->size].reverse_direction = 0;
    tlist->trains[tlist->size].cur_node = 0;
    tlist->trains[tlist->size].path = track_path_new();
    track_path_add(&tlist->trains[tlist->size].path, id == 55 ? 140 : 130, 1e9);
    train_data_t train_data = init_train_data_a();
    tlist->trains[tlist->size].acc = 0;
    tlist->trains[tlist->size].acc_start = 0;
    tlist->trains[tlist->size].acc_end = 0;
    tlist->trains[tlist->size].cur_offset = train_data.train_length[id];
    tlist->trains[tlist->size].cur_stop_node = 0;

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
    GET_TRAIN_OLD_SPEED = 3,
    TRAIN_EXISTS = 4,
    SENSOR_READING = 5,
    GET_TRAIN_TIMES = 6,
    TRAIN_LAST_SENSOR = 7,
    SET_TRAIN_REVERSE = 8,
    GET_TRAIN_REVERSE = 9,
    SHOULD_UPDATE_TRAIN_STATE = 10,
    GET_CUR_NODE = 11,
    GET_CUR_OFFSET = 12,
    SET_CUR_NODE = 13,
    SET_CUR_OFFSET = 14,
    ROUTE_TRAIN = 15,
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

void set_train_speed_handler(train_data_t* train_data, train_t* t, uint64_t speed)
{
    int old_speed = t->speed;
    if (old_speed == 0 && speed > 0) {
        t->acc = train_data->acc_start[t->id][speed];
        t->acc_start = timer_get_ms();
        t->acc_end = timer_get_ms() + train_data->starting_time[t->id][speed];
    } else if (old_speed > 0 && speed == 0) {
        t->acc = train_data->acc_stop[t->id][old_speed];
        t->acc_start = timer_get_ms();
        t->acc_end = timer_get_ms() + train_data->stopping_time[t->id][old_speed];
    }
    t->speed = speed;
    t->old_speed = old_speed;
    t->speed_time_begin = timer_get_ms();
}

uint64_t train_get_speed(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2], response;
    buf[0] = GET_TRAIN_SPEED;
    buf[1] = train;
    int64_t ret = send(train_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    return response;
}

uint64_t train_get_old_speed(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2], response;
    buf[0] = GET_TRAIN_OLD_SPEED;
    buf[1] = train;
    int64_t ret = send(train_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    return response;
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

int train_set_cur_node(uint64_t train, int node)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[10];
    buf[0] = SET_CUR_NODE;
    buf[1] = train;
    buf[2] = node;
    int64_t ret = send(train_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
    return 0;
}

int train_set_cur_offset(uint64_t train, int offset)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[10];
    buf[0] = SET_CUR_OFFSET;
    buf[1] = train;
    i2a(offset, &buf[2]);
    int64_t ret = send(train_task_tid, buf, 10, NULL, 0);
    ASSERT(ret >= 0, "send failed");
    return 0;
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

void train_model_notifier()
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");
    int cur_time = time();
    char c = SHOULD_UPDATE_TRAIN_STATE;

    for (;;) {
        int64_t ret = send(train_task_tid, &c, 1, NULL, 0);
        ASSERT(ret >= 0, "send failed");

        cur_time += 5;
        delay_until(cur_time);
    }
}

int segments_in_path_up_to(int* segments, track_node* track, track_path_t* path, int start_node, int end_node)
{
    int segment_index = 0;
    for (int i = start_node; i < end_node; ++i) {
        track_node* cur_node = &track[path->nodes[i]];
        track_node* nxt_node = &track[path->nodes[i + 1]];
        for (int j = 0; j < 2; ++j) {
            if (cur_node->enters_seg[j] >= 0 && cur_node->edge[j].dest == nxt_node) {
                segments[segment_index++] = cur_node->enters_seg[j];
            }
        }

        if (cur_node->reverse == nxt_node) {
            int reachable_segments[8];
            int num_reachable_segments = reachable_segments_within_distance(reachable_segments, track, path->nodes[i], 300);
            for (int j = 0; j < num_reachable_segments; ++j) {
                segments[segment_index++] = reachable_segments[j];
            }
        }
    }
    return segment_index;
}

void route_train_handler(track_node* track, train_t* t, train_data_t* train_data, track_path_t* path)
{
    int segments_to_release[16];
    int num_segments_to_release = segments_in_path_up_to(segments_to_release, track, &t->path, 0, t->path.path_length - 1);
    for (int i = 0; i < num_segments_to_release - 1; ++i) {
        if (state_is_reserved(segments_to_release[i]) == (int)t->id) {
            state_release_segment(segments_to_release[i], t->id);
        }
    }

    if (t->cur_node == t->path.path_length - 2 && abs(t->cur_offset - t->path.distances[t->cur_node]) < 50) {
        t->cur_offset = 0;
        t->cur_node++;
    }

    track_node* old_node = &track[t->path.nodes[t->cur_node]];
    t->path = *path;

    t->cur_node = 0;
    if (&track[t->path.nodes[0]] == old_node->reverse) {
        marklin_reverse(t->id);
        t->reverse_direction = !t->reverse_direction;
        t->cur_offset = train_data->train_length[t->id] - t->cur_offset;
        while (t->cur_offset >= t->path.distances[t->cur_node]) {
            t->cur_offset -= t->path.distances[t->cur_node++];
        }
    }

    for (int i = t->path.path_length - 2; i >= 0; --i) {
        track_node node = track[t->path.nodes[i]];
        if (node.type == NODE_BRANCH) {
            char switch_type = (get_node_index(track, node.edge[DIR_STRAIGHT].dest) == t->path.nodes[i + 1]) ? S : C;

            create_switch_task(node.num, switch_type);
        }
    }
    int64_t ret = create(1, &deactivate_solenoid_task);
    ASSERTF(ret >= 0, "create failed: %d", ret);

    t->stop_node = t->path.stop_node;
    t->stop_distance_offset = t->path.stop_distance_offset;
    t->cur_stop_node = 0;
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
    // putc(MARKLIN, 64);

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
            ASSERTF(t != NULL, "train %d not found.", train);
            char response = t->speed;
            ret = reply_char(caller_tid, response);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_TRAIN_OLD_SPEED: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ASSERTF(t != NULL, "train %d not found.", train);
            char response = t->old_speed;
            ret = reply_char(caller_tid, response);
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

            for (int i = 0; i < trainlist.size; ++i) {
                train_t* train = &trainlist.trains[i];

                // spurious reading from same sensor
                if (train->last_sensor == node_index) {
                    continue;
                }
                int distance_to_sensor = 1e9;
                if (train->path.nodes[train->cur_node] == node_index) {
                    distance_to_sensor = train->cur_offset;
                } else if (train->cur_node + 1 < train->path.path_length && train->path.nodes[train->cur_node + 1] == node_index) {
                    distance_to_sensor = train->cur_offset - train->path.distances[train->cur_node]; // neg number
                }

                if (distance_to_sensor > -SENSOR_PREDICTION_WINDOW && distance_to_sensor < SENSOR_PREDICTION_WINDOW) {
                    // printf(CONSOLE, "attributing sensor %s to train %d\r\n", track[node_index].name, train->id);
                    sprintf(train_times, "distance delta: %dmm", distance_to_sensor);

                    if (distance_to_sensor < 0) {
                        train->cur_node++;
                    }
                    ASSERT(train->path.nodes[train->cur_node] == node_index, "train isn't at sensor node");
                    train->cur_offset = train->reverse_direction ? train_data.reverse_stopping_distance_offset[train->id] : 25;

                    // release reservations behind sensor
                    int segments_to_release[16];
                    int num_segments_to_release = segments_in_path_up_to(segments_to_release, track, &train->path, 0, train->cur_node);
                    for (int j = 0; j < num_segments_to_release - 1; ++j) { // don't release segment that sensor is on
                        state_release_segment(segments_to_release[j], train->id);
                    }

                    // train->sensors = get_reachable_sensors(track, node_index);
                    train->last_sensor = node_index;
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

                if (t->acc_end < interval_start) {
                    t->acc = 0;
                }

                int acc_start = min(interval_end, max(interval_start, t->acc_start));
                int acc_end = min(interval_end, max(interval_start, t->acc_end));

                int acc_duration = acc_end - acc_start;
                int new_speed_duration = interval_end - acc_end;
                int old_speed_duration = acc_start - interval_start;

                int acc_distance = t->inst_speed * acc_duration + t->acc * acc_duration * acc_duration / 2;
                t->inst_speed += t->acc * (acc_end - acc_start);
                // in a short route, the train doesn't need the full decceleration time, so manually stop if inst_speed < 0
                if (t->inst_speed < 0 && t->acc < 0) {
                    t->acc_end = 0;
                    t->inst_speed = 0;
                    t->acc = 0;
                }

                int new_speed_distance = new_speed_duration * train_data.speed[t->id][t->speed];
                int old_speed_distance = old_speed_duration * train_data.speed[t->id][t->old_speed];
                t->distance_overflow_nm += acc_distance + (new_speed_distance + old_speed_distance) * 1000;

                int update_distance = t->distance_overflow_nm / 1000000;
                t->distance_overflow_nm -= update_distance * 1000000;

                t->cur_offset += update_distance;
                while (t->cur_offset >= t->path.distances[t->cur_node]) {
                    t->cur_offset -= t->path.distances[t->cur_node++];
                }

                // handle stopping.
                if (t->path.nodes[t->cur_node] == t->stop_node && t->cur_offset >= t->stop_distance_offset) {
                    marklin_set_speed(t->id, 0);

                    set_train_speed_handler(&train_data, t, 0);

                    t->stop_node = -1;
                }

                if (t->cur_stop_node < t->path.stop_node_count) {
                    if (t->path.nodes[t->cur_node] == t->path.stop_nodes[t->cur_stop_node] && t->cur_offset >= t->path.stop_offsets[t->cur_stop_node]) {
                        int64_t reverse_task_id = create(1, &train_reverse_task);
                        char args[10];
                        args[0] = t->id;
                        args[1] = t->path.stop_dest_nodes[t->cur_stop_node];
                        i2a(t->path.stop_dest_offsets[t->cur_stop_node], &args[2]);
                        int64_t ret = send(reverse_task_id, args, 10, NULL, 0);
                        ASSERT(ret >= 0, "send failed");
                        t->cur_stop_node++;
                    }
                }

                int distance_ahead = 0;
                int cur_node_index = t->cur_node;
                while (cur_node_index < t->path.path_length - 1 && distance_ahead < RESERVATION_LOOKAHEAD_DISTANCE + t->cur_offset) {
                    distance_ahead += t->path.distances[cur_node_index];
                    cur_node_index++;
                }

                int segments_to_reserve[16];
                int num_segments_to_reserve = segments_in_path_up_to(segments_to_reserve, track, &t->path, t->cur_node, cur_node_index);
                for (int j = 0; j < num_segments_to_reserve; ++j) {
                    int conflict_seg = segments_to_reserve[j];
                    uint64_t reserver = state_is_reserved(conflict_seg);
                    if (reserver && reserver != t->id) {
                        // printf(CONSOLE, "collision detected: %d %d", t->id, reserver);

                        train_t* train_one = trainlist_find(&trainlist, reserver);
                        train_t* train_two = t;
                        track_path_t train_one_path, train_two_path;

                        // printf(CONSOLE, "%s, %d, %s, %d\r\n", track[t->path.nodes[t->cur_node]].name, t->cur_offset, track[train_one->path.nodes[train_one->cur_node]].name, train_one->cur_offset);

                        // CASE 1: keep 1 on path, reroute 2
                        train_one_path = get_shortest_path(track, train_one, train_one->path.dest, train_one->path.dest_offset, NO_FORBIDDEN_SEGMENT);
                        train_two_path = get_shortest_path(track, train_two, train_two->path.dest, train_two->path.dest_offset, conflict_seg);
                        int case_one_dist = 0;
                        case_one_dist += train_one_path.path_length == 0 ? (int)1e9 : train_one_path.path_distance;
                        case_one_dist += train_two_path.path_length == 0 ? (int)1e9 : train_two_path.path_distance;

                        // CASE 2: keep 2 on path, reroute 1
                        train_one_path = get_shortest_path(track, train_one, train_one->path.dest, train_one->path.dest_offset, conflict_seg);
                        train_two_path = get_shortest_path(track, train_two, train_two->path.dest, train_two->path.dest_offset, NO_FORBIDDEN_SEGMENT);
                        int case_two_dist = 0;
                        case_two_dist += train_one_path.path_length == 0 ? (int)1e9 : train_one_path.path_distance;
                        case_two_dist += train_two_path.path_length == 0 ? (int)1e9 : train_two_path.path_distance;

                        // printf(CONSOLE, "case one dist: %d, case two dist: %d\r\n", case_one_dist, case_two_dist);

                        if (case_one_dist < case_two_dist) {
                            route_train_handler(track, train_two, &train_data, &train_two_path);
                            marklin_set_speed(train_one->id, 0);
                            set_train_speed_handler(&train_data, train_one, 0);
                        } else {
                            route_train_handler(track, train_one, &train_data, &train_one_path);
                            marklin_set_speed(train_two->id, 0);
                            set_train_speed_handler(&train_data, train_two, 0);
                        }
                    } else {
                        state_reserve_segment(segments_to_reserve[j], t->id);
                    }
                }
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
        case SET_CUR_NODE: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            int node = buf[2];
            t->cur_node = node;
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SET_CUR_OFFSET: {
            uint64_t train = buf[1];
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");
            int offset = a2i(&buf[2], 10);
            t->cur_offset = offset;
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case ROUTE_TRAIN: {
            uint64_t train = buf[1];
            int dest = buf[2];
            int offset = a2i(buf + 3, 10);
            train_t* t = trainlist_find(&trainlist, train);
            ASSERT(t != NULL, "train not found");

            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");

            track_path_t path = get_shortest_path(track, t, dest, offset, NO_FORBIDDEN_SEGMENT);
            route_train_handler(track, t, &train_data, &path);

            // for (int i = 0; i < t->path.path_length - 1; ++i) {
            //     printf(CONSOLE, "%s \r\n", track[t->path.nodes[i]].name);
            // }
            // puts(CONSOLE, "\r\n");

            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }
}
