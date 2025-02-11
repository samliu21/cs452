#ifndef _switch_h_
#define _switch_h_

#include "common.h"

typedef enum {
    S = 'S',
    C = 'C',
} switchstate_t;

typedef struct tswitch_t {
    uint64_t id;
    switchstate_t state;
} tswitch_t;

typedef struct switchlist_t {
    int n_switches;
    tswitch_t* switches;
} switchlist_t;

switchlist_t switch_createlist(tswitch_t* switches);
void switch_set_state(switchlist_t* tlist, uint64_t id, switchstate_t state);
tswitch_t* switch_find(switchlist_t* tlist, uint64_t id);

#endif
