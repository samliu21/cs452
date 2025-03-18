#ifndef _track_algo_h_
#define _track_algo_h_

#include "common.h"
#include "track_node.h"
#include "track_path.h"

#define NO_FORBIDDEN_SEGMENT -1

typedef struct train_t train_t;

int get_node_index(track_node* track_begin, track_node* node);
track_path_t get_shortest_path(track_node* track, train_t* train, int dest, int node_offset, int forbidden_seg, int is_full_speed);
int reachable_segments_within_distance(int* reachable_nodes, track_node* track, int src, int max_distance);

#endif
