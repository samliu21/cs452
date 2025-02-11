#include "state_server.h"
#include "charqueue.h"
#include "marklin.h"
#include "name_server.h"
#include "rpi.h"
#include "switch.h"
#include "syscall_func.h"
#include "train.h"

void state_set_speed(uint64_t state_task_tid, uint64_t train, uint64_t speed)
{
    char buf[3];
    buf[0] = SET_TRAIN_SPEED;
    buf[1] = speed;
    buf[2] = train;
    int64_t ret = send(state_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

uint64_t state_get_speed(uint64_t state_task_tid, uint64_t train)
{
    char buf[2], response[2];
    buf[0] = GET_TRAIN_SPEED;
    buf[1] = train;
    int64_t ret = send(state_task_tid, buf, 2, response, 2);
    ASSERT(ret >= 0, "send failed");
    return response[1];
}

void state_set_recent_sensor(uint64_t state_task_tid, char bank, char sensor)
{
    char buf[3];
    buf[0] = SET_RECENT_SENSOR;
    buf[1] = bank;
    buf[2] = sensor;
    int64_t ret = send(state_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void state_get_recent_sensors(uint64_t state_task_tid, char* response)
{
    char c = GET_RECENT_SENSORS;
    int64_t ret = send(state_task_tid, &c, 1, response, 4 * NUM_RECENT_SENSORS + 1);
    ASSERT(ret >= 0, "send failed");
}

void state_set_switch(uint64_t state_task_tid, uint64_t sw, char d)
{
    char buf[3];
    buf[0] = SET_SWITCH;
    buf[1] = sw;
    buf[2] = d;
    int64_t ret = send(state_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void state_get_switches(uint64_t state_task_tid, char* response)
{
    char c = GET_SWITCHES;
    int64_t ret = send(state_task_tid, &c, 1, response, 128);
    ASSERT(ret >= 0, "send failed");
}

void state_task()
{
    int64_t ret;

    ret = register_as(STATE_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");
    int64_t marklin_task_tid = who_is(MARKLIN_TASK_NAME);
    ASSERT(marklin_task_tid >= 0, "who_is failed");

    train_t trains[5];
    trainlist_t trainlist = train_createlist(trains);
    train_add(&trainlist, 55);

    tswitch_t switch_buf[64];
    switchlist_t switchlist = switch_createlist(switch_buf);

    charqueuenode sensornodes[4 * NUM_RECENT_SENSORS + 1];
    charqueue sensorqueue = charqueue_new(sensornodes, 4 * NUM_RECENT_SENSORS + 1);

    uint64_t caller_tid;
    char buf[3];
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
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_RECENT_SENSORS: {
            char response[4 * NUM_RECENT_SENSORS + 1];
            int sz = 0;
            charqueuenode* head = sensorqueue.head;
            for (int i = 0; i < 4 * NUM_RECENT_SENSORS; ++i) {
                if (head == NULL) {
                    break;
                }
                response[i] = head->data;
                head = head->next;
                sz++;
            }
            response[sz] = 0;
            ret = reply(caller_tid, response, sz + 1);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SET_SWITCH: {
            uint64_t sw = buf[1];
            char d = buf[2];
            switch_set_state(&switchlist, sw, d);
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_SWITCHES: {
            char buf[128];
            int sz = 0;
            for (int i = 0; i < switchlist.n_switches; ++i) {
                tswitch_t s = switchlist.switches[i];

                char numbuf[5];
                ui2a(s.id, 10, numbuf);
                strcpy(buf + sz, numbuf);
                sz += strlen(numbuf);
                buf[sz++] = ':';
                buf[sz++] = s.state;
                buf[sz++] = ';';
                buf[sz++] = ' ';
            }
            buf[sz] = 0;
            ret = reply(caller_tid, buf, sz + 1);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }

    exit();
}
