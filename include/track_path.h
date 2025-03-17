#ifndef _track_path_h_
#define _track_path_h_

#include "track_data.h"

typedef struct track_path_t {
    int nodes[TRACK_MAX];
    int distances[TRACK_MAX];
    int path_length;
    int stop_node;
    int stop_distance_offset;
    int stop_nodes[8];
    int stop_offsets[8];
    int stop_dest_nodes[8];
    int stop_dest_offsets[8];
	int stop_node_count;
} track_path_t;

track_path_t track_path_new();
int track_path_add(track_path_t* path, int node, int dist);
int track_path_lookahead(track_path_t* path, int node, int lookahead);

#endif
