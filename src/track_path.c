#include "track_path.h"
#include "rpi.h"

track_path_t track_path_new()
{
    track_path_t path;
    path.path_length = 0;
    return path;
}

int track_path_add(track_path_t* path, int node, int dist)
{
    ASSERT(path->path_length < TRACK_MAX, "path length exceeds maximum");
    path->nodes[path->path_length] = node;
    path->distances[path->path_length] = dist;
    return ++path->path_length;
}

int track_path_lookahead(track_path_t* path, int node, int lookahead)
{
    while (path->distances[node] <= lookahead) {
        lookahead -= path->distances[node];
        node++;
    }
    return node;
}
