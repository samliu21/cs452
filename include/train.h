#ifndef _train_h_
#define _train_h_

#include "common.h"

typedef struct train_t {
    uint64_t id;
    uint64_t speed;
} train_t;

typedef struct trainlist_t {
    train_t* trains;
    int size;
} trainlist_t;

trainlist_t train_createlist(train_t* trains);
void train_add(trainlist_t* tlist, uint64_t id);
void train_set_speed(trainlist_t* tlist, uint64_t id, uint64_t speed);
train_t* train_find(trainlist_t* tlist, uint64_t id);

void state_set_speed(uint64_t train_task_tid, uint64_t train, uint64_t speed);
uint64_t state_get_speed(uint64_t train_task_tid, uint64_t train);
int state_train_exists(uint64_t train_task_tid, uint64_t train);

void train_task();

#endif
