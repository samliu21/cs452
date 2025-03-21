#include "track_path.h"
#include "rpi.h"
#include "uart_server.h"

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

void track_path_debug(track_path_t* path, track_node* track)
{
    puts(CONSOLE, "----------------------------------\r\n");
    printf(CONSOLE, "path length: %d\r\n", path->path_length);
    for (int i = 0; i < path->path_length; ++i) {
        printf(CONSOLE, "node: %s, dist: %d\r\n", track[path->nodes[i]].name, path->distances[i]);
    }
    puts(CONSOLE, "\r\n");
    printf(CONSOLE, "stop node: %s, stop offset: %d\r\n", track[path->stop_node].name, path->stop_distance_offset);
    for (int i = 0; i < path->stop_node_count; ++i) {
        printf(CONSOLE, "stop node %d: %s, stop offset %d\r\n", i, track[path->stop_nodes[i]].name, path->stop_offsets[i]);
    }
    puts(CONSOLE, "----------------------------------\r\n");
}
