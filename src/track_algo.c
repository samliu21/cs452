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

    int reverse_node = get_node_index(track, track[src].reverse);
    nodes[nodes_pos].weight = 0;
    nodes[nodes_pos].id = reverse_node;
    pq_pi_add(&pq, &nodes[nodes_pos++]);
    dist[reverse_node] = 0;
    prev[reverse_node] = -1;

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

            for (int i = path_length - 1; i >= 0; --i) {
                int dist_between = (i == 0) ? 1e9 : dist[path_reverse[i - 1]] - dist[path_reverse[i]];
                // for reverse edge, update distance accordingly for reservation calculations
                if (dist_between == reverse_edge_weight) {
                    if (track[path_reverse[i]].type == NODE_MERGE) {
                        dist_between = train_data.train_length[train->id] + 2 * REVERSE_OVER_MERGE_OFFSET;
                    } else if (track[path_reverse[i]].type == NODE_EXIT) {
                        printf(CONSOLE, "path contains reverse at exit %s\r\n", track[path_reverse[i]].name);
                        dist_between = -train_data.train_length[train->id];
                    } else {
                        ASSERTF(0, "reversing at invalid node %s", track[path_reverse[i]].name);
                    }
                }
                track_path_add(&path, path_reverse[i], dist_between);
            }

            int total_path_distance = dist[dest] + node_offset - train->cur_offset;
            int fully_stop_fully_start = train_data.starting_distance[train->id][train->speed] + train_data.stopping_distance[train->id][train->speed];

            if (total_path_distance < fully_stop_fully_start) {
                int lo = 0, hi = train_data.starting_time[train->id][train->speed], accelerate_for = -1;
                while (lo <= hi) {
                    int md = (lo + hi) / 2; // allow train to accelerate for "md" ms

                    int64_t distance_acc = train_data.acc_start[train->id][train->speed] * md * md / 2;
                    int64_t vf = train_data.acc_start[train->id][train->speed] * md;
                    int64_t distance_dec = vf * vf / (-train_data.acc_stop[train->id][train->speed] * 2);
                    int distance_travelled = (distance_acc + distance_dec) / 1000000;
                    if (distance_travelled > total_path_distance) {
                        hi = md - 1;
                    } else {
                        lo = md + 1;
                        accelerate_for = md;
                    }
                }

                ASSERTF(accelerate_for >= 0 && accelerate_for <= train_data.starting_time[train->id][train->speed], "accelerate_for of %d is invalid for distance %d", accelerate_for, total_path_distance);

                int vf = train_data.acc_start[train->id][train->speed] * accelerate_for / 1000;
                stopping_distance = vf * vf / (-train_data.acc_stop[train->id][train->speed] * 2);
            }

            // starting from node BEFORE the dest node, find the node and time offset at which we send stop command
            for (int i = 1; i < path_length; ++i) {
                int cur_node = path_reverse[i];
                int distance_from_end = dist[dest] - dist[cur_node] + node_offset;
                if (distance_from_end >= stopping_distance) {
                    path.stop_node = cur_node;
                    path.stop_distance_offset = distance_from_end - stopping_distance;
                    break;
                }
                if (i == path_length - 1) {
                    ASSERT(0, "could not find stop node");
                }
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

int reachable_segments_within_distance(int* reachable_segments, track_node* track, int src, int max_distance)
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

    int num_reachable_segments = 0;

    while (!pq_pi_empty(&pq)) {
        pi_t* pi = pq_pi_pop(&pq);
        int node = pi->id;
        int weight = pi->weight;
        if (weight > max_distance) {
            break;
        }
        if (dist[node] < weight) {
            continue;
        }

        for (int i = 0; i < 2; ++i) {
            if (track[node].enters_seg[i] >= 0) {
                reachable_segments[num_reachable_segments++] = track[node].enters_seg[i];
            }
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
    }

    return num_reachable_segments;
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
