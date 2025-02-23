#ifndef _track_algo_h_
#define _track_algo_h_

#include "track_node.h"
#include "track_path.h"

int get_node_index(track_node* track_begin, track_node* node);
track_path_t get_shortest_path(track_node* track, int src, int dest);

#endif