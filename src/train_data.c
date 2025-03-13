#include "train_data.h"

train_data_t init_train_data_a()
{
    train_data_t data;

    data.reverse_stopping_distance_offset = 125;

    // train 55
    data.train_length[55] = 212;

    data.speed[55][0] = 0;
    data.speed[55][6] = 89;
    data.speed[55][10] = 274;

    data.stopping_distance[55][6] = 75;
    data.stopping_distance[55][10] = 330;

    // data.starting_time[55][6] =
    data.starting_time[55][10] = 4567;

    data.stopping_time[55][6] = 2 * data.stopping_distance[55][6] * 1000 / data.speed[55][10];
    data.stopping_time[55][10] = 2 * data.stopping_distance[55][10] * 1000 / data.speed[55][10];

    data.acc_stop[55][6] = -1 * data.speed[55][6] * data.speed[55][6] / (2 * data.stopping_distance[55][6]);
    data.acc_stop[55][10] = -1 * data.speed[55][10] * data.speed[55][10] / (2 * data.stopping_distance[55][10]);

    // data.acc_start[55][6] =
    data.acc_start[55][10] = data.speed[55][10] * 1000 / data.starting_time[55][10];

    data.reverse_edge_weight[55] = 1000;

    // train 77
    // data.speed[77][0] = 0;
    // data.speed[77][6] = 133;
    // data.speed[77][10] = 337;

    // data.stopping_distance[77][6][0] = 190;
    // data.stopping_distance[77][6][1] = data.stopping_distance[77][6][0] + data.reverse_stopping_distance_offset;
    // data.stopping_distance[77][10][0] = 630;
    // data.stopping_distance[77][10][1] = data.stopping_distance[77][10][0] + data.reverse_stopping_distance_offset;

    // data.stopping_time[77][6][0] = 2857;
    // data.stopping_time[77][6][1] = 4737;
    // data.stopping_time[77][10][0] = 3739;
    // data.stopping_time[77][10][1] = 4481;

    // data.train_length[77] = 225;

    // data.acc_stop[77][6] = -90;
    // data.acc_stop[77][10] = -90;
    // data.acc_start[77][6] = 90;
    // data.acc_start[77][10] = 90;

    // data.reverse_edge_weight[77][6] = 1000;
    // data.reverse_edge_weight[77][10] = 1000;

    return data;
}
