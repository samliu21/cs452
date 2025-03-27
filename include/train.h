#ifndef _train_h_
#define _train_h_

#include "common.h"
#include "track_algo.h"
#include "track_node.h"

#define REVERSE_OVER_MERGE_OFFSET 100

typedef struct train_t {
    uint64_t id;
    uint64_t speed;
    uint64_t old_speed;
    int speed_time_begin;
    int stop_node;
    int stop_distance_offset;
    int last_sensor;
    int reverse_direction;

    // train model
    int cur_node;
    int cur_offset;
    track_path_t path;
    int cur_seg;
    int acc;
    int acc_start;
    int acc_end;
    int inst_speed;

    int distance_overflow_nm;
    int avoid_seg_on_reroute;

    int random_reroute;
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
int train_last_sensor(uint64_t train);
void train_set_reverse(uint64_t train);
int train_get_reverse(uint64_t train);
int train_get_cur_node(uint64_t train);
int train_get_cur_offset(uint64_t train);
void train_route(uint64_t train, int dest, int offset);
void train_random_reroute(uint64_t train);
int train_get_dest(uint64_t train);
int train_get_player();
void train_set_player(char player);

void train_model_notifier();
void train_task();

#endif