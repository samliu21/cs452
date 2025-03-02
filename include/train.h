#ifndef _train_h_
#define _train_h_

#include "common.h"
#include "track_algo.h"
#include "track_node.h"

typedef struct train_t {
    uint64_t id;
    uint64_t speed;
    reachable_sensors_t sensors;
    int stop_node;
    int stop_time_offset;
    int last_sensor;
    int reverse_direction;
} train_t;

typedef struct trainlist_t {
    train_t* trains;
    int size;
} trainlist_t;

trainlist_t trainlist_create(train_t* trains);
void trainlist_add(trainlist_t* tlist, uint64_t id);
train_t* trainlist_find(trainlist_t* tlist, uint64_t id);

void train_set_speed(uint64_t train, uint64_t speed);
uint64_t train_get_speed(uint64_t train);
int train_exists(uint64_t train);
void train_sensor_reading(track_node* track, char* sensor);
void train_get_times(char* response);
int train_last_sensor(uint64_t train);
void train_set_stop_node(uint64_t train, int node, int time_offset);
void train_set_reverse(uint64_t train);
int train_get_reverse(uint64_t train);

void train_task();

#endif