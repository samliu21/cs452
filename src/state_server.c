#include "state_server.h"
#include "charqueue.h"
#include "marklin.h"
#include "name_server.h"
#include "priority_queue_pi.h"
#include "rpi.h"
#include "switch.h"
#include "syscall_func.h"
#include "track_data.h"
#include "track_seg.h"
#include "train.h"
#include "uart_server.h"

#define MAX_RESERVATIONS 200

typedef enum {
    SET_RECENT_SENSOR = 1,
    GET_RECENT_SENSORS = 2,
    SET_SWITCH = 3,
    GET_SWITCHES = 4,
    GET_SWITCH_DATA = 5,
    SWITCH_EXISTS = 6,
    NEXT_SENSOR = 7,
    IS_RESERVED = 8,
    RESERVE_SEGMENT = 9,
    RELEASE_SEGMENT = 10,
    GET_RESERVATIONS = 11,
    FORBID_SEGMENT = 12,
    GET_FORBIDDEN_SEGMENTS = 13,
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
    ASSERT(ret >= 0, "set recent sensor send failed");
}

void state_get_recent_sensors(char* response)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char c = GET_RECENT_SENSORS;
    int64_t ret = send(state_task_tid, &c, 1, response, 4 * NUM_RECENT_SENSORS + 1);
    ASSERT(ret >= 0, "get recent sensors send failed");
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
    ASSERT(ret >= 0, "set switch send failed");
}

void state_get_switches(char* response)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char c = GET_SWITCHES;
    int64_t ret = send(state_task_tid, &c, 1, response, 256);
    ASSERT(ret >= 0, "get switches send failed");
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
    ASSERT(ret >= 0, "switch exists send failed");
    return response;
}

int state_next_sensor(int sensor)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = NEXT_SENSOR;
    buf[1] = sensor;
    char response;
    int64_t ret = send(state_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "next sensor send failed");
    if (ret == 0)
        return -1;
    return response;
}

int state_is_reserved(int segment)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = IS_RESERVED;
    buf[1] = segment;
    char response;
    int64_t ret = send(state_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "is reserved send failed");
    return response;
}

int state_reserve_segment(int segment, int train)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[3];
    buf[0] = RESERVE_SEGMENT;
    buf[1] = segment;
    buf[2] = train;
    int64_t ret = send(state_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "reserve segment send failed");
    return 0;
}

int state_release_segment(int segment, int train)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[3];
    buf[0] = RELEASE_SEGMENT;
    buf[1] = segment;
    buf[2] = train;
    int64_t ret = send(state_task_tid, buf, 3, NULL, 0);
    ASSERT(ret >= 0, "release segment send failed");
    return 0;
}

void state_get_reservations(char* response, int train)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = GET_RESERVATIONS;
    buf[1] = train;
    int64_t ret = send(state_task_tid, buf, 2, response, 1024);
    ASSERT(ret >= 0, "get reservations send failed");
}

int state_forbid_segment(int segment)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = FORBID_SEGMENT;
    buf[1] = segment;
    int64_t ret = send(state_task_tid, buf, 2, NULL, 0);
    ASSERT(ret >= 0, "forbid segment send failed");
    return 0;
}

void state_get_forbidden_segments(char* response)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[1];
    buf[0] = GET_FORBIDDEN_SEGMENTS;
    int64_t ret = send(state_task_tid, buf, 1, response, TRACK_SEGMENTS_MAX);
    ASSERT(ret >= 0, "get forbidden segments send failed");
    return;
}

void state_task()
{
    int64_t ret;

    ret = register_as(STATE_TASK_NAME);
    ASSERT(ret >= 0, "register_as failed");

    tswitch_t switch_buf[64];
    switchlist_t switchlist = switch_createlist(switch_buf);
    for (int i = 0; i < switchlist.n_switches; ++i) {
        create_switch_task(switchlist.switches[i].id, switchlist.switches[i].state);
    }
    ret = create(1, &deactivate_solenoid_task);
    ASSERTF(ret >= 0, "create failed: %d", ret);

    charqueuenode sensornodes[4 * NUM_RECENT_SENSORS + 1];
    charqueue sensorqueue = charqueue_new(sensornodes, 4 * NUM_RECENT_SENSORS + 1);

    track_node track[TRACK_MAX];
#ifdef TRACKA
    init_tracka(track);
#else
    init_trackb(track);
#endif

    int reservations[TRACK_SEGMENTS_MAX];
    for (int i = 0; i < TRACK_SEGMENTS_MAX; ++i) {
        reservations[i] = 0;
    }

    char forbidden_segments[TRACK_SEGMENTS_MAX];
    memset(forbidden_segments, 0, TRACK_SEGMENTS_MAX);

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
            char bitset[256];
            memset(bitset, 0, 256);
            for (int i = 0; i < switchlist.n_switches; ++i) {
                tswitch_t s = switchlist.switches[i];
                bitset[s.id] = s.state;
            }
            ret = reply(caller_tid, bitset, 256);
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
        case NEXT_SENSOR: {
            int sensor = buf[1];
            track_node* cur_node = track[sensor].edge[DIR_AHEAD].dest;
            while (cur_node->type != NODE_SENSOR && cur_node->type != NODE_EXIT) {
                if (cur_node->type == NODE_BRANCH) {
                    int dir = (switch_find(&switchlist, cur_node->num)->state == S)
                        ? DIR_STRAIGHT
                        : DIR_CURVED;
                    cur_node = cur_node->edge[dir].dest;
                } else {
                    cur_node = cur_node->edge[DIR_AHEAD].dest;
                }
            }
            if (cur_node->type == NODE_EXIT) {
                ret = reply_empty(caller_tid);
                ASSERT(ret >= 0, "reply failed");
                break;
            }
            int next_sensor = get_node_index(track, cur_node);
            ASSERTF(next_sensor >= 0 && next_sensor < TRACK_MAX, "next sensor has invalid index %d", next_sensor);
            ret = reply_char(caller_tid, next_sensor);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case IS_RESERVED: {
            int segment = buf[1];
            ret = reply_char(caller_tid, reservations[segment]);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case RESERVE_SEGMENT: {
            int segment = buf[1];
            int train = buf[2];
            ASSERTF(!reservations[segment] || reservations[segment] == train, "train %d tried to reserve segment %d, which is reserved by train %d", train, segment, reservations[segment]);
            reservations[segment] = train;
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case RELEASE_SEGMENT: {
            int segment = buf[1];
            int train = buf[2];
            ASSERTF(reservations[segment] == train || !reservations[segment], "train %d tried to release segment %d, which is reserved by train %d", train, segment, reservations[segment]);
            reservations[segment] = 0;
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_RESERVATIONS: {
            int train = buf[1];
            char response[1024];
            memset(response, 0, 1024);
            for (int i = 0; i < TRACK_SEGMENTS_MAX; ++i) {
                if (reservations[i] == train) {
                    char buf[32];
                    memset(buf, 0, 32);
                    sprintf(buf, "%d; ", i);
                    strcat(response, buf);
                }
            }
            ret = reply(caller_tid, response, 1024);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case FORBID_SEGMENT: {
            int segment = buf[1];
            forbidden_segments[segment] = 1 - forbidden_segments[segment];
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case GET_FORBIDDEN_SEGMENTS: {
            char response[TRACK_SEGMENTS_MAX];
            memcpy(response, forbidden_segments, TRACK_SEGMENTS_MAX);
            ret = reply(caller_tid, response, TRACK_SEGMENTS_MAX);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }

    exit();
}
