#ifndef _track_algo_h_
#define _track_algo_h_

#include "track_node.h"
#include "track_path.h"

int get_node_index(track_node* track_begin, track_node* node);
track_path_t get_shortest_path(track_node* track, int src, int dest, int speed_level);
track_path_t get_closest_node(track_node* track, int src, int* dests, int n);

////////////////////////////////////////////////////////////////////////////////

#define MAX_REACHABLE_SENSORS 32

typedef struct reachable_sensors_t {
    int size;
    int sensors[MAX_REACHABLE_SENSORS];
    int distances[MAX_REACHABLE_SENSORS];
} reachable_sensors_t;

reachable_sensors_t get_reachable_sensors(track_node* track, int src_sensor);

#endif
