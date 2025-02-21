#ifndef _uintmap_h_
#define _uintmap_h_

#include "common.h"

// map<uint64_t, uint64_t>
typedef struct uintmap_t {
    uint64_t keys[NUM_TASKS];
    uint64_t values[NUM_TASKS];
    int num_keys;
} uintmap_t;

uintmap_t uintmap_new();
void uintmap_set(uintmap_t* map, uint64_t key, uint64_t value);
uint64_t uintmap_get(uintmap_t* map, uint64_t key);
void uintmap_remove(uintmap_t* map, uint64_t key);
int uintmap_contains(uintmap_t* map, uint64_t key);
void uintmap_increment(uintmap_t* map, uint64_t key, uint64_t value);

#endif
