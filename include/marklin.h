#ifndef _marklin_h_
#define _marklin_h_

#include "common.h"

void marklin_set_speed(uint64_t train, uint64_t speed);
void marklin_reverse(uint64_t train);
void marklin_set_switch(uint64_t sw, char d);

void train_reverse_task();
void train_reverse_after_stop_task();
void deactivate_solenoid_task();

#endif
