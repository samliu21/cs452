#include "state_server.h"
#include "charqueue.h"
#include "name_server.h"
#include "rpi.h"
#include "syscall_func.h"
#include "train.h"

void state_set_speed(uint64_t state_task_tid, uint64_t train, uint64_t speed)
{
    char buf[3];
    buf[0] = SET_TRAIN_SPEED;
    buf[1] = speed;
    buf[2] = train;
    send(state_task_tid, buf, 3, NULL, 0);
}

uint64_t state_get_speed(uint64_t state_task_tid, uint64_t train)
{
    char buf[2], response[2];
    buf[0] = GET_TRAIN_SPEED;
    buf[1] = train;
    send(state_task_tid, buf, 2, response, 2);
    return response[1];
}

void state_set_recent_sensor(uint64_t state_task_tid, char bank, char sensor)
{
    char buf[3];
    buf[0] = SET_RECENT_SENSOR;
    buf[1] = bank;
    buf[2] = sensor;
    send(state_task_tid, buf, 3, NULL, 0);
}

void state_get_recent_sensors(uint64_t state_task_tid, char* response)
{
    char c = GET_RECENT_SENSORS;
    int64_t ret = send(state_task_tid, &c, 1, response, 32);
    ASSERT(ret >= 0, "send failed");
}

void state_task()
{
    int64_t res = register_as(STATE_TASK_NAME);
    ASSERT(res >= 0, "register_as failed");

    train_t trains[5];
    trainlist_t trainlist = train_createlist(trains);
    train_add(&trainlist, 55);

    charqueuenode sensornodes[32];
    charqueue sensorqueue = charqueue_new(sensornodes, 32);

    uint64_t caller_tid;
    char buf[3];
    for (;;) {
        receive(&caller_tid, buf, 3);
        switch (buf[0]) {
        case SET_TRAIN_SPEED: {
            uint64_t speed = buf[1];
            uint64_t train = buf[2];
            train_t* t = train_find(&trainlist, train);
            if (t == NULL) {
                reply_num(caller_tid, 1);
            } else {
                t->speed = speed;
                reply_num(caller_tid, 0);
            }
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
            reply(caller_tid, response, 2);
            break;
        }
        case SET_RECENT_SENSOR: {
            if (sensorqueue.size >= NUM_RECENT_SENSORS * 4) {
                for (int i = 0; i < 4; ++i) {
                    charqueue_pop(&sensorqueue);
                }
            }
            char bank = buf[1];
            char sensor = buf[2];
            charqueue_add(&sensorqueue, 'A' + bank);
            charqueue_add(&sensorqueue, '0' + (sensor / 10));
            charqueue_add(&sensorqueue, '0' + (sensor % 10));
            charqueue_add(&sensorqueue, ' ');
            reply_empty(caller_tid);
            break;
        }
        case GET_RECENT_SENSORS: {
            char response[32];
            memset(response, 0, 32);
            charqueuenode* head = sensorqueue.head;
            for (int i = 0; i < 4 * NUM_RECENT_SENSORS; ++i) {
                if (head == NULL) {
                    break;
                }
                response[i] = head->data;
                head = head->next;
            }
            reply(caller_tid, response, 32);
            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }

    exit();
}
