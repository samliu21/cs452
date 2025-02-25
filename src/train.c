#include "train.h"
#include "name_server.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include "track_data.h"
#include "track_node.h"
#include <stdlib.h>

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
} train_task_request_t;

void state_set_speed(uint64_t train_task_tid, uint64_t train, uint64_t speed)
{
    char buf[3];
    buf[0] = SET_TRAIN_SPEED;
    buf[1] = speed;
    buf[2] = train;
    int64_t ret = send(train_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

uint64_t state_get_speed(uint64_t train_task_tid, uint64_t train)
{
    char buf[2], response[2];
    buf[0] = GET_TRAIN_SPEED;
    buf[1] = train;
    int64_t ret = send(train_task_tid, buf, 2, response, 2);
    ASSERT(ret >= 0, "send failed");
    return response[1];
}

int state_train_exists(uint64_t train_task_tid, uint64_t train)
{
    char buf[2];
    buf[0] = TRAIN_EXISTS;
    buf[1] = train;
    char response;
    int64_t ret = send(train_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    return response;
}

void state_sensor_reading(uint64_t train_task_tid, track_node* track, char* sensor)
{
    int node_index = name_to_node_index(track, sensor);
    ASSERTF(node_index >= 0, "invalid sensor name: '%s'", sensor);
    char buf[2];
    buf[0] = SENSOR_READING;
    buf[1] = node_index;
    int64_t ret = send(train_task_tid, buf, 2, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void train_task()
{
    int64_t ret;

    ret = register_as(TRAIN_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");

    train_t trains[5];
    trainlist_t trainlist = train_createlist(trains);
    train_add(&trainlist, 55);
    // train_add(&trainlist, 54);

    track_node track[TRACK_MAX];
    init_tracka(track);

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
                break;
            }

            for (int i = 0; i < trainlist.size; ++i) {
                train_t* train = &trainlist.trains[i];
                for (int j = 0; j < train->sensors.size; ++j) {
                    if (train->sensors.sensors[j] == node_index) {
                        train->sensors = get_reachable_sensors(track, node_index);
                        goto end;
                    }
                }
            }
        end:
            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }
}
