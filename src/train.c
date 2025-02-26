#include "train.h"
#include "common.h"
#include "name_server.h"
#include "state_server.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include "timer.h"
#include "track_data.h"
#include "track_node.h"
#include "uart_server.h"
#include <stdlib.h>

#define NUM_LOOP_SENSORS 8

trainlist_t train_createlist(train_t* trains)
{
    trainlist_t tlist;
    tlist.trains = trains;
    tlist.size = 0;
    return tlist;
}

void train_add(trainlist_t* tlist, uint64_t id)
{
    tlist->trains[tlist->size].id = id;
    tlist->trains[tlist->size].speed = 0;
    tlist->trains[tlist->size].sensors.size = 0;
    tlist->size++;
}

void train_set_speed(trainlist_t* tlist, uint64_t id, uint64_t speed)
{
    train_t* t = train_find(tlist, id);
    t->speed = speed;
}

train_t* train_find(trainlist_t* tlist, uint64_t id)
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
    TRAIN_LOOP_NEXT = 5,
} train_task_request_t;

void state_set_speed(uint64_t train, uint64_t speed)
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

uint64_t state_get_speed(uint64_t train)
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

int state_train_exists(uint64_t train)
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

void state_sensor_reading(track_node* track, char* sensor)
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

int train_loop_next(uint64_t train)
{
    int64_t train_task_tid = who_is(TRAIN_TASK_NAME);
    ASSERT(train_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = TRAIN_LOOP_NEXT;
    buf[1] = train;
    char response[4];
    int64_t ret = send(train_task_tid, buf, 2, response, 4);
    if (ret == 0) {
        return -1;
    }
    return a2ui(response, 10);
}

void train_task()
{
    int64_t ret;

    ret = register_as(TRAIN_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");

    train_t trains[5];
    trainlist_t trainlist = train_createlist(trains);
    train_add(&trainlist, 55);
    int last_time;
    // train_add(&trainlist, 54);

    track_node track[TRACK_MAX];
    init_tracka(track);

    // A3 C13 E7 D7 D9 E12 C6 B15
    int loop_sensors[NUM_LOOP_SENSORS] = { 2, 44, 70, 54, 56, 75, 37, 30 };

    uint64_t caller_tid;
    char buf[3];
    int has_received_initial_sensor = 0;
    for (;;) {
        ret = receive(&caller_tid, buf, 3);
        ASSERT(ret >= 0, "receive failed");

        switch (buf[0]) {
        case SET_TRAIN_SPEED: {
            uint64_t speed = buf[1];
            uint64_t train = buf[2];
            train_t* t = train_find(&trainlist, train);
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
            train_t* t = train_find(&trainlist, train);
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
            train_t* t = train_find(&trainlist, train);
            ret = reply_char(caller_tid, t == NULL ? 0 : 1);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SENSOR_READING: {
            int node_index = buf[1];

            // if the train has not received a sensor reading yet, set the reachable sensors from the initial reading
            // otherwise, find the train expecting this sensor reading and update its reachable sensors
            if (!has_received_initial_sensor) {
                has_received_initial_sensor = 1;
                trainlist.trains[0].sensors = get_reachable_sensors(track, node_index);
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
                        char buf[64];
                        sprintf(buf, "time difference: %d", t_actual - t_pred);
                        state_set_train_times(buf);
                        last_time = timer_get_ms();

                        train->sensors = get_reachable_sensors(track, node_index);
                        goto sensor_reading_end;
                    }
                }
            }

        sensor_reading_end:
            reply_empty(caller_tid);
            break;
        }
        case TRAIN_LOOP_NEXT: {
            uint64_t train = buf[1];
            train_t* t = train_find(&trainlist, train);
            if (t->sensors.size == 0) {
                // train has not hit a sensor yet.
                
                ret = reply_empty(caller_tid);
                ASSERT(ret >= 0, "reply failed");
                break;
            }
            int sensor = -1;
            for (int i = 0; i < t->sensors.size; ++i) {
                for (int j = 0; j < NUM_LOOP_SENSORS; ++j) {
                    if (t->sensors.sensors[i] == loop_sensors[j]) {
                        sensor = loop_sensors[j];
                    }
                }
            }
            if (sensor >= 0) {
                char response[4];
                ui2a(sensor, 10, response);
                ret = reply(caller_tid, response, 4);
                ASSERT(ret >= 0, "reply failed");
                break;
            }
            // train is not in the loop.
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }
}
