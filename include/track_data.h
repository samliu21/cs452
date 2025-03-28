#ifndef _track_data_h_
#define _track_data_h_

#include "track_node.h"

// The track initialization functions expect an array of this size.
#define TRACK_MAX 144
#define NUM_FORBIDDEN_DESTS 12
#define MAX_SWITCH_INDEX 157

int name_to_node_index(track_node* track, char* name);
void init_tracka(track_node* track);
void init_trackb(track_node* track);
void init_forbidden_destsa(int* forbidden_dests);
void init_switch_left_straighta(char* switch_left_straight);

#endif
