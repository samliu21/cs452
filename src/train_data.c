#include "train_data.h"

train_data_t init_train_data_a()
{
    train_data_t data;
    data.speed[55][6] = 89;
    data.speed[55][10] = 274;

    data.reverse_stopping_distance_offset = 125;

    data.stopping_distance[55][6][0] = 75;
    data.stopping_distance[55][6][1] = data.stopping_distance[55][6][0] + data.reverse_stopping_distance_offset;
    data.stopping_distance[55][10][0] = 340;
    data.stopping_distance[55][10][1] = data.stopping_distance[55][10][0] + data.reverse_stopping_distance_offset;
    return data;
}
