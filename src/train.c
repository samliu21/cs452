#include "train.h"
#include "clock_server.h"
#include "common.h"
#include "marklin.h"
#include "name_server.h"
#include "state_server.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include "timer.h"
#include "track_data.h"
#include "track_node.h"
#include "uart_server.h"
#include <stdlib.h>

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
    tlist->trains[tlist->size].sensors.size = 0;
    tlist->trains[tlist->size].last_sensor = -1;
    tlist->trains[tlist->size].stop_node = -1;
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

void get_train_times(char* response)
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

void set_stop_node(uint64_t train, int node, int time_offset)
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

void train_task()
{
    int64_t ret;

    ret = register_as(TRAIN_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");

    train_t trains[5];
    trainlist_t trainlist = trainlist_create(trains);
    trainlist_add(&trainlist, 55);
    int last_time;
    // trainlist_add(&trainlist, 54);

    track_node track[TRACK_MAX];
    init_tracka(track);

    uint64_t caller_tid;
    char buf[32];
    int has_received_initial_sensor = 0;
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
            } else {
                t->speed = speed;
                ret = reply_num(caller_tid, 0);
            }
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

            for (int i = 0; i < trainlist.size; ++i) {
                if (node_index == trainlist.trains[i].stop_node) {
                    int64_t stop_task_tid = create(1, &train_stop_task);
                    ASSERT(stop_task_tid >= 0, "create failed");

                    char time_offset_buf[32];
                    time_offset_buf[0] = trainlist.trains[i].id;
                    ui2a(trainlist.trains[i].stop_time_offset, 10, time_offset_buf + 1);
                    ret = send(stop_task_tid, time_offset_buf, 32, NULL, 0);
                    ASSERT(ret >= 0, "send failed");

                    trainlist.trains[i].stop_node = -1;
                }
            }

            // if the train has not received a sensor reading yet, set the reachable sensors from the initial reading
            // otherwise, find the train expecting this sensor reading and update its reachable sensors
            if (!has_received_initial_sensor) {
                has_received_initial_sensor = 1;
                trainlist.trains[0].sensors = get_reachable_sensors(track, node_index);
                trainlist.trains[0].last_sensor = node_index;
                last_time = timer_get_ms();
                goto sensor_reading_end;
            }

            for (int i = 0; i < trainlist.size; ++i) {
                train_t* train = &trainlist.trains[i];
                for (int j = 0; j < train->sensors.size; ++j) {
                    if (train->sensors.sensors[j] == node_index) {
                        // print predicted and actual times
                        int t_pred = train->sensors.distances[j] * 1000 / TRAIN_SPEED;
                        int t_actual = timer_get_ms() - last_time;
                        int t_diff = t_actual - t_pred;

                        sprintf(train_times, "time delta: %dms, distance delta: %dmm", t_diff, t_diff * TRAIN_SPEED / 1000);

                        last_time = timer_get_ms();

                        train->sensors = get_reachable_sensors(track, node_index);
                        train->last_sensor = node_index;
                        goto sensor_reading_end;
                    }
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
        default:
            ASSERT(0, "invalid request");
        }
    }
}
