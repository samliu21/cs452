#include "track_algo.h"
#include "priority_queue_pi.h"
#include "rpi.h"
#include "track_data.h"
#include "track_node.h"
#include "train.h"
#include "train_data.h"
#include "uart_server.h"
#include <stdlib.h>

int get_node_index(track_node* track_begin, track_node* node)
{
    return node - track_begin;
}

void add_to_queue(priority_queue_pi_t* pq, int* dist, int* prev, pi_t* nodes, int* nodes_pos, int prev_node, int cur_node, int weight)
{
    int dist_cand = dist[prev_node] + weight;
    if (dist_cand < dist[cur_node]) {
        nodes[*nodes_pos].weight = dist_cand;
        nodes[*nodes_pos].id = cur_node;
        pq_pi_add(pq, &nodes[(*nodes_pos)++]);
        dist[cur_node] = dist_cand;
        prev[cur_node] = prev_node;
    }
}

track_path_t get_shortest_path(track_node* track, train_t* train, int dest, int node_offset)
{
    priority_queue_pi_t pq = pq_pi_new();
    pi_t nodes[256];
    int nodes_pos = 0;
    int prev[TRACK_MAX], dist[TRACK_MAX];
    for (int i = 0; i < TRACK_MAX; ++i) {
        dist[i] = 1e9;
    }

    train_data_t train_data = init_train_data_a();

    int src = train->path.nodes[train->cur_node];
    nodes[nodes_pos].weight = 0;
    nodes[nodes_pos].id = src;
    pq_pi_add(&pq, &nodes[nodes_pos++]);
    dist[src] = 0;
    prev[src] = -1;

    int speed = train_data.speed[train->id][train->speed];
    int stopping_distance = train_data.stopping_distance[train->id][train->speed];
    int reverse_edge_weight = train_data.reverse_edge_weight[train->id];

    track_path_t path = track_path_new();

    while (!pq_pi_empty(&pq)) {
        pi_t* pi = pq_pi_pop(&pq);
        int node = pi->id;
        int weight = pi->weight;
        if (node == dest) {
            int path_reverse[TRACK_MAX];
            int path_length = 0;
            while (node != -1) {
                path_reverse[path_length++] = node;
                node = prev[node];
            }
            path_reverse[path_length++] = node;

            // starting from node BEFORE the dest node, find the node and time offset at which we send stop command
            for (int i = 1; i < path_length; ++i) {
                int cur_node = path_reverse[i];
                int distance_from_end = dist[dest] - dist[cur_node] + node_offset;
                if (distance_from_end >= stopping_distance) {
                    path.stop_node = cur_node;
                    path.stop_time_offset = (distance_from_end - stopping_distance) * 1000 / speed;
                    path.stop_distance_offset = distance_from_end - stopping_distance;
                    break;
                }
                if (i == path_length - 1) {
                    ASSERT(0, "could not find stop node");
                }
            }
            for (int i = path_length - 1; i >= 0; --i) {
                int dist_between = (i == 0) ? 1e9 : dist[path_reverse[i - 1]] - dist[path_reverse[i]];
                // for reverse edge, update distance to be train length + offset for reservation calculations
                if (dist_between == reverse_edge_weight) {
                    dist_between = train_data.train_length[train->id] + REVERSE_OVER_MERGE_OFFSET;
                }
                track_path_add(&path, path_reverse[i], dist_between);
            }
            break;
        }
        if (dist[node] < weight) {
            continue;
        }

        switch (track[node].type) {
        case NODE_BRANCH: {
            track_edge straight_edge = track[node].edge[DIR_STRAIGHT];
            track_edge curved_edge = track[node].edge[DIR_CURVED];
            int node_straight = get_node_index(track, straight_edge.dest);
            int node_curved = get_node_index(track, curved_edge.dest);
            add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_straight, straight_edge.dist);
            add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_curved, curved_edge.dist);
            break;
        }

        case NODE_SENSOR:
        case NODE_MERGE:
        case NODE_ENTER: {
            // one edge
            track_edge edge = track[node].edge[DIR_AHEAD];
            int node_ahead = get_node_index(track, edge.dest);
            add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_ahead, edge.dist);
            break;
        }

        case NODE_EXIT:
            break;

        default:
            ASSERTF(0, "invalid node: %d, type: %d", node, track[node].type);
        }

        if (track[node].type == NODE_EXIT || track[node].type == NODE_MERGE) {
            int reverse_node = get_node_index(track, track[node].reverse);
            add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, reverse_node, reverse_edge_weight);
        }
    }

    return path;
}

track_path_t get_closest_node(track_node* track, int src, int* dests, int n)
{
    priority_queue_pi_t pq = pq_pi_new();
    pi_t nodes[256];
    int nodes_pos = 0;
    int prev[TRACK_MAX], dist[TRACK_MAX];
    for (int i = 0; i < TRACK_MAX; ++i) {
        dist[i] = 1e9;
    }

    nodes[nodes_pos].weight = 0;
    nodes[nodes_pos].id = src;
    pq_pi_add(&pq, &nodes[nodes_pos++]);
    dist[src] = 0;
    prev[src] = -1;

    track_path_t path = track_path_new();
    int dest = -1;

    while (!pq_pi_empty(&pq)) {
        pi_t* pi = pq_pi_pop(&pq);
        int node = pi->id;
        int weight = pi->weight;
        for (int i = 0; i < n; ++i) {
            if (node == dests[i]) {
                dest = dests[i];
            }
        }
        if (dest >= 0) {
            int path_reverse[TRACK_MAX];
            int path_length = 0;
            while (node != -1) {
                path_reverse[path_length++] = node;
                node = prev[node];
            }
            for (int i = path_length - 1; i >= 0; --i) {
                int dist_between = (i == 0) ? -1 : dist[path_reverse[i - 1]] - dist[path_reverse[i]];
                track_path_add(&path, path_reverse[i], dist_between);
            }
            break;
        }
        if (dist[node] < weight) {
            continue;
        }

        switch (track[node].type) {
        case NODE_BRANCH: {
            // two edges
            track_edge straight_edge = track[node].edge[DIR_STRAIGHT];
            track_edge curved_edge = track[node].edge[DIR_CURVED];
            int node_straight = get_node_index(track, straight_edge.dest);
            int node_curved = get_node_index(track, curved_edge.dest);
            add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_straight, straight_edge.dist);
            add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_curved, curved_edge.dist);
            break;
        }

        case NODE_SENSOR:
        case NODE_MERGE:
        case NODE_ENTER: {
            // one edge
            track_edge edge = track[node].edge[DIR_AHEAD];
            int node_ahead = get_node_index(track, edge.dest);
            add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_ahead, edge.dist);
            break;
        }

        case NODE_EXIT:
            break;

        default:
            ASSERTF(0, "invalid node: %d, type: %d", node, track[node].type);
        }
    }

    return path;
}
