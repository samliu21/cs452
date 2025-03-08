#ifndef _track_path_h_
#define _track_path_h_

#include "track_data.h"

typedef struct track_path_t {
    int nodes[TRACK_MAX];
    int distances[TRACK_MAX];
    int path_length;
	int stop_node;
	int stop_time_offset;
} track_path_t;

track_path_t track_path_new();
int track_path_add(track_path_t* path, int node, int dist);
int track_path_lookahead(track_path_t* path, int node, int lookahead);

#endif
