#ifndef _train_data_h_
#define _train_data_h_

#include "common.h"

typedef struct train_data_t {
    int64_t speed[256][16]; // [train][speed]
    int64_t stopping_distance[256][16]; // [train][speed][is_reversed]
    int64_t starting_distance[256][16];
    int64_t stopping_time[256][16];
    int64_t starting_time[256][16];
    int64_t reverse_stopping_distance_offset[256];
    int64_t train_length[256];
    int64_t acc_stop[256][16];
    int64_t acc_start[256][16];
    int64_t start_node[256];
    int64_t start_seg[256];
} train_data_t;

train_data_t init_train_data_a();

#endif
