#ifndef _train_data_h_
#define _train_data_h_

#include "common.h"

typedef struct train_data_t {
    uint64_t speed[256][16]; // [train][speed]
    uint64_t stopping_distance[256][16][2]; // [train][speed][is_reversed]
    uint64_t reverse_stopping_distance_offset;
} train_data_t;

train_data_t init_train_data_a();

#endif
