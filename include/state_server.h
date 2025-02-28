#ifndef _state_server_h_
#define _state_server_h_

#include "common.h"

#define NUM_RECENT_SENSORS 10

void state_task();

void state_set_recent_sensor(char bank, char sensor);
void state_get_recent_sensors(char* response);
void state_set_switch(uint64_t sw, char d);
void state_get_switches(char* response);
int state_switch_exists(uint64_t sw);
int state_next_sensor(int sensor);

#endif
