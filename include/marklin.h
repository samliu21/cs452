#ifndef _marklin_h_
#define _marklin_h_

#include "common.h"

void marklin_set_speed(uint64_t marklin_task_tid, uint64_t train, uint64_t speed);

void train_reverse_task();

#endif
