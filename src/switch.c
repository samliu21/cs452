#include "switch.h"
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
