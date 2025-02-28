#include "switch.h"
#include "marklin.h"
#include "state_server.h"
#include "syscall_asm.h"
#include "syscall_func.h"
#include <stdlib.h>

switchlist_t switch_createlist(tswitch_t* switches)
{
    // default switch states
    char default_states[256];
    memset(default_states, S, 256);
    default_states[7] = default_states[18] = C;

    // create switchlist
    switchlist_t tlist;
    int cnt = 0;
    for (int i = 1; i <= 18; ++i) {
        switches[cnt].id = i;
        switches[cnt].state = default_states[i];
        cnt++;
    }
    for (int i = 153; i <= 156; ++i) {
        switches[cnt].id = i;
        switches[cnt].state = default_states[i];
        cnt++;
    }
    tlist.switches = switches;
    tlist.n_switches = cnt;
    return tlist;
}

void switch_set_state(switchlist_t* tlist, uint64_t id, switchstate_t state)
{
    tswitch_t* s = switch_find(tlist, id);
    if (s == NULL) {
        return;
    }
    s->state = state;
}

// void switch_print(charqueue* q, switchlist_t* tlist)
// {
//     enqueue_s(q, "switches: [ ");
//     for (int i = 0; i < tlist->n_switches; ++i) {
//         tswitch_t* s = &tlist->switches[i];
//         enqueue_sf(q, "%d:%c; ", s->id, s->state);
//     }
//     enqueue_s(q, "]\r\n");
// }

tswitch_t* switch_find(switchlist_t* tlist, uint64_t id)
{
    for (int i = 0; i < tlist->n_switches; ++i) {
        if (tlist->switches[i].id == id) {
            return &tlist->switches[i];
        }
    }
    return NULL;
}

void fix_middle_switches(int switch_num, switchstate_t dir)
{
    if (dir == C) {
        int corresponding_switch_num = 0;
        if (switch_num == 153) {
            corresponding_switch_num = 154;
        } else if (switch_num == 154) {
            corresponding_switch_num = 153;
        } else if (switch_num == 155) {
            corresponding_switch_num = 156;
        } else if (switch_num == 156) {
            corresponding_switch_num = 155;
        }
        if (corresponding_switch_num) {
            state_set_switch(corresponding_switch_num, S);

            marklin_set_switch(corresponding_switch_num, S);
        }
    }
}

void set_switch_task()
{
    int64_t ret;
    char buf[2];
    uint64_t caller_tid;

    ret = receive(&caller_tid, buf, 2);
    ASSERT(ret >= 0, "receive failed");
    ret = reply_empty(caller_tid);
    ASSERT(ret >= 0, "reply_empty failed");

    uint64_t switch_num = buf[0];
    char switch_type = buf[1];

    fix_middle_switches(switch_num, switch_type);
    state_set_switch(switch_num, switch_type);
    marklin_set_switch(switch_num, switch_type);

    syscall_exit();
}

void create_switch_task(int switch_num, char switch_type)
{
    int64_t task_tid = create(1, &set_switch_task);
    ASSERT(task_tid >= 0, "create failed");
    char buf[2];
    buf[0] = switch_num;
    buf[1] = switch_type;
    int64_t ret = send(task_tid, buf, 2, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}
