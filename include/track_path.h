#ifndef _track_path_h_
#define _track_path_h_

#include "track_data.h"

typedef struct track_path_t {
    int nodes[TRACK_MAX];
    int distances[TRACK_MAX];
    int path_length;
    int stop_node;
    int stop_distance_offset;
    int dest;
    int dest_offset;
    int path_distance;
} track_path_t;

track_path_t track_path_new();
int track_path_add(track_path_t* path, int node, int dist);
int track_path_lookahead(track_path_t* path, int node, int lookahead);
void track_path_debug(track_path_t* path, track_node* track);

#endif
