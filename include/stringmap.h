#ifndef _stringmap_h_
#define _stringmap_h_

#include "common.h"

#define MAX_KEY_SIZE 32

// map<string, uint64_t> with max size MAX_KEY_SIZE (including null character)
typedef struct stringmap_t {
    char keys[NUM_TASKS][MAX_KEY_SIZE];
    uint64_t values[NUM_TASKS];
    int num_keys;
} stringmap_t;

stringmap_t stringmap_new();
void stringmap_set(stringmap_t* map, char* key, uint64_t value);
uint64_t stringmap_get(stringmap_t* map, char* key);
void stringmap_remove(stringmap_t* map, char* key);

#endif
