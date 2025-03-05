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

int state_next_sensor(int sensor)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = NEXT_SENSOR;
    buf[1] = sensor;
    char response;
    int64_t ret = send(state_task_tid, buf, 2, &response, 1);
    ASSERT(ret >= 0, "send failed");
    if (ret == 0)
        return -1;
    return response;
}

int state_is_reserved(int segment, uint32_t check_time)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[6];
    buf[0] = IS_RESERVED;
    buf[1] = segment;
    ui2cbuf(check_time, &(buf[2]));
    char response;
    int64_t ret = send(state_task_tid, buf, 6, &response, 1);
    ASSERT(ret >= 0, "send failed");
    if (ret == 0)
        return -1;
    return response;
}

int is_reserved(priority_queue_pi_t *seg_queue, uint32_t check_time) {
    pi_t * node = pq_pi_peek(seg_queue);
    if (!node) return 0;
    while (node->next && node->next->weight < check_time) {
        node = node->next;
    }
    if (node->weight + node->id > check_time) {
        return 1;
    }
    return 0;
}

int state_reserve_segment(int segment, uint32_t start, uint32_t duration)
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");

    char buf[10];
    buf[0] = NEXT_SENSOR;
    buf[1] = segment;
    ui2cbuf(start, &(buf[2]));
    ui2cbuf(duration, &(buf[6]));
    int64_t ret = send(state_task_tid, buf, 10, NULL, 0);
    ASSERT(ret >= 0, "send failed");
    return 0;
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

    track_node track[TRACK_MAX];
#ifdef TRACKA
    init_tracka(track);
#else
    init_trackb(track);
#endif

    pi_t reservation_nodes[MAX_RESERVATIONS][TRACK_SEGMENTS_MAX];
    int reservation_next_nodes[TRACK_SEGMENTS_MAX];
    priority_queue_pi_t reservations[TRACK_SEGMENTS_MAX];
    for (int i = 0; i < TRACK_SEGMENTS_MAX; ++i) {
        reservation_next_nodes[i] = 0;
        reservations[i] = pq_pi_new();
    }

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
            uint32_t check_time = cbuf2ui(&buf[2]);
            ret = reply_char(caller_tid, is_reserved(&(reservations[segment]), check_time));
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        case RESERVE_SEGMENT: {
            int segment = buf[1];
            uint32_t start = cbuf2ui(&buf[2]);
            uint32_t duration = cbuf2ui(&buf[6]);
            ASSERTF(!is_reserved(&(reservations[segment]), start), "tried to double-reserve segment %d", segment);
            pi_t * node = &(reservation_nodes[reservation_next_nodes[segment]++][segment]);
            if (reservation_next_nodes[segment] >= MAX_RESERVATIONS) {
                reservation_next_nodes[segment] = 0;
            }
            node->weight = start;
            node->id = duration;
            pq_pi_add(&(reservations[segment]), node);
            ASSERTF(reservations[segment].size <= MAX_RESERVATIONS, "segment %d has too many reservations", segment);
            ASSERTF(!(node->next) || node->next->weight > start + duration, "tried to double-reserve segment %d", segment);
            ret = reply_empty(caller_tid);
            ASSERT(ret >= 0, "reply failed");
            break;
        }
        default:
            ASSERT(0, "invalid request");
        }
    }

    exit();
}
