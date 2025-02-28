#ifndef _train_h_
#define _train_h_

#include "common.h"
#include "track_algo.h"
#include "track_node.h"

#define TRAIN_SPEED_HIGH_LEVEL 10
#define TRAIN_SPEED_LOW_LEVEL 6
#define TRAIN_SPEED_HIGH 274 // mm/s
#define TRAIN_SPEED_LOW 89

#define TRAIN_STOPPING_DISTANCE_HIGH 340 // mm
#define TRAIN_STOPPING_DISTANCE_LOW 75

typedef struct train_t {
    uint64_t id;
    uint64_t speed;
    reachable_sensors_t sensors;
    int stop_node;
    int stop_time_offset;
    int last_sensor;
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
void get_train_times(char* response);
int train_last_sensor(uint64_t train);
void set_stop_node(uint64_t train, int node, int time_offset);

void train_task();

#endif