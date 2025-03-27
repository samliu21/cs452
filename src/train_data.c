#include "train_data.h"

train_data_t init_train_data_a()
{
    train_data_t data;

    // mm
    data.reverse_stopping_distance_offset[55] = 125;

    // mm
    data.train_length[55] = 212;

    // mm/s
    data.speed[55][0] = 0;
    data.speed[55][10] = 274;

    // mm
    data.stopping_distance[55][10] = 330;

    // ms
    data.starting_time[55][10] = 4567;

    // mm
    data.starting_distance[55][10] = data.speed[55][10] * data.starting_time[55][10] / 2000;

    // ms
    data.stopping_time[55][10] = 2 * data.stopping_distance[55][10] * 1000 / data.speed[55][10];

    // mm/s^2
    data.acc_stop[55][10] = -1 * data.speed[55][10] * data.speed[55][10] / (2 * data.stopping_distance[55][10]);

    // mm/s^2
    data.acc_start[55][10] = data.speed[55][10] * 1000 / data.starting_time[55][10];

    data.start_node[55] = 140;
    data.start_seg[55] = 4;

    // train 77

    data.reverse_stopping_distance_offset[77] = 148;

    // mm
    data.train_length[77] = 225;

    // mm/s
    data.speed[77][0] = 0;
    data.speed[77][10] = 337;

    // mm
    data.stopping_distance[77][10] = 590;

    // ms
    data.starting_time[77][10] = 4065;

    // mm
    data.starting_distance[77][10] = data.speed[77][10] * data.starting_time[77][10] / 2000;

    // ms
    data.stopping_time[77][10] = 2 * data.stopping_distance[77][10] * 1000 / data.speed[77][10];

    // mm/s^2
    data.acc_stop[77][10] = -1 * data.speed[77][10] * data.speed[77][10] / (2 * data.stopping_distance[77][10]);

    // mm/s^2
    data.acc_start[77][10] = data.speed[77][10] * 1000 / data.starting_time[77][10];

    data.start_node[77] = 130;
    data.start_seg[77] = 7;

    // train 58

    data.reverse_stopping_distance_offset[58] = 138;

    // mm
    data.train_length[58] = 208;

    // mm/s
    data.speed[58][0] = 0;
    data.speed[58][10] = 309;

    // mm
    data.stopping_distance[58][10] = 425;

    // ms
    data.starting_time[58][10] = 3172;

    // mm
    data.starting_distance[58][10] = data.speed[58][10] * data.starting_time[58][10] / 2000;

    // ms
    data.stopping_time[58][10] = 2 * data.stopping_distance[58][10] * 1000 / data.speed[58][10];

    // mm/s^2
    data.acc_stop[58][10] = -1 * data.speed[58][10] * data.speed[58][10] / (2 * data.stopping_distance[58][10]);

    // mm/s^2
    data.acc_start[58][10] = data.speed[58][10] * 1000 / data.starting_time[58][10];

    data.start_node[58] = 128;
    data.start_seg[58] = 9;

    return data;
}
