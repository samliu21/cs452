#ifndef _state_server_h_
#define _state_server_h_

#include "common.h"

typedef enum {
    SET_TRAIN_SPEED = 1,
    GET_TRAIN_SPEED = 2,
    SET_RECENT_SENSOR = 3,
    GET_RECENT_SENSORS = 4,
    SET_SWITCH = 5,
    GET_SWITCHES = 6,
} state_server_request_t;

#define NUM_RECENT_SENSORS 5

void state_task();

void state_set_speed(uint64_t state_task_tid, uint64_t train, uint64_t speed);
uint64_t state_get_speed(uint64_t state_task_tid, uint64_t train);
void state_set_recent_sensor(uint64_t state_task_tid, char bank, char sensor);
void state_get_recent_sensors(uint64_t state_task_tid, char* response);
void state_set_switch(uint64_t state_task_tid, uint64_t sw, char d);
void state_get_switches(uint64_t state_task_tid, char* response);

#endif
