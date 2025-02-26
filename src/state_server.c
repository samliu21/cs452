#include "state_server.h"
#include "charqueue.h"
#include "marklin.h"
#include "name_server.h"
#include "rpi.h"
#include "switch.h"
#include "syscall_func.h"
#include "train.h"
#include "uart_server.h"

typedef enum {
    SET_RECENT_SENSOR = 1,
    GET_RECENT_SENSORS = 2,
    SET_SWITCH = 3,
    GET_SWITCHES = 4,
    SWITCH_EXISTS = 5,
    SET_TRAIN_TIMES = 6,
    GET_TRAIN_TIMES = 7,
} state_server_request_t;

void state_set_recent_sensor(char bank, char sensor)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[3];
    buf[0] = SET_RECENT_SENSOR;
    buf[1] = bank;
    buf[2] = sensor;
    int64_t ret = send(state_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void state_get_recent_sensors(char* response)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char c = GET_RECENT_SENSORS;
    int64_t ret = send(state_task_tid, &c, 1, response, 4 * NUM_RECENT_SENSORS + 1);
    ASSERT(ret >= 0, "send failed");
}

void state_set_switch(uint64_t sw, char d)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[3];
    buf[0] = SET_SWITCH;
    buf[1] = sw;
    buf[2] = d;
    int64_t ret = send(state_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void state_get_switches(char* response)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char c = GET_SWITCHES;
    int64_t ret = send(state_task_tid, &c, 1, response, 128);
    ASSERT(ret >= 0, "send failed");
}

int state_switch_exists(uint64_t sw)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = SWITCH_EXISTS;
    buf[1] = sw;
    char response;
    int64_t ret = send(state_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    return response;
}

void state_set_train_times(char* times)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[2 + strlen(times)];
    buf[0] = SET_TRAIN_TIMES;
    strcpy(buf + 1, times);
    int64_t ret = send(state_task_tid, buf, 2 + strlen(times), NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void state_get_train_times(char* response)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char c = GET_TRAIN_TIMES;
    int64_t ret = send(state_task_tid, &c, 1, response, 256);
    ASSERT(ret >= 0, "send failed");
}

void state_task()
{
    int64_t ret;

    ret = register_as(STATE_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");

    tswitch_t switch_buf[64];
    switchlist_t switchlist = switch_createlist(switch_buf);

    charqueuenode sensornodes[4 * NUM_RECENT_SENSORS + 1];
    charqueue sensorqueue = charqueue_new(sensornodes, 4 * NUM_RECENT_SENSORS + 1);

    char train_times[256];
    memset(&train_times, 0, 256);

    uint64_t caller_tid;
    char buf[256];
    for (;;) {
        ret = receive(&caller_tid, buf, 256);
        ASSERT(ret >= 0, "receive failed");

        switch (buf[0]) {
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
        case SWITCH_EXISTS: {
            uint64_t sw = buf[1];
            tswitch_t* s = switch_find(&switchlist, sw);
            ret = reply_char(caller_tid, s == NULL ? 0 : 1);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case SET_TRAIN_TIMES: {
            char* times = buf + 1;
            strcpy(train_times, times);
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_TRAIN_TIMES: {
            ret = reply(caller_tid, train_times, strlen(train_times) + 1);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }

    exit();
}
