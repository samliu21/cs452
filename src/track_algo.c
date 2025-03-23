#include "track_algo.h"
#include "priority_queue_pi.h"
#include "rpi.h"
#include "state_server.h"
#include "track_data.h"
#include "track_node.h"
#include "track_seg.h"
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

int get_short_stop_distance(train_data_t* train_data, train_t* train, int total_path_distance)
{
    int speed = (train->speed > 0) ? train->speed : train->old_speed;
    int acc_start = train_data->acc_start[train->id][speed];
    int acc_stop = train_data->acc_stop[train->id][speed];

    // unphysical magic formula
    if (total_path_distance < 350 && train->id == 55) {
        int acc_factor = 350 / total_path_distance;
        acc_start /= acc_factor;
        acc_stop *= acc_factor;
    }
    if ((train->id == 77 || train->id == 58) && total_path_distance > 300) {
        if (total_path_distance < 600) {
            acc_start *= 8;
            acc_start /= 3;
        } else {
            acc_start *= 3;
            acc_start /= 2;
        }
    }

    int lo = 0, hi = train_data->starting_time[train->id][speed];

    int stop_distance = -1;
    while (lo <= hi) {
        int md = (lo + hi) / 2; // allow train to accelerate for "md" ms

        int64_t distance_acc = acc_start * md * md / 2;
        int64_t vf = acc_start * md;
        int64_t distance_dec = vf * vf / (-acc_stop * 2);
        int distance_travelled = (distance_acc + distance_dec) / 1000000;
        if (distance_travelled >= total_path_distance) {
            stop_distance = distance_dec;
            hi = md - 1;
        } else {
            lo = md + 1;
        }
    }

    stop_distance /= 1000000;
    ASSERTF(stop_distance >= 0 && stop_distance <= total_path_distance, "stop_distance of %d is invalid for distance %d", stop_distance, total_path_distance);
    return stop_distance;
}

track_path_t get_shortest_path(track_node* track, train_t* train, int dest, int node_offset, int forbidden_seg)
{
    priority_queue_pi_t pq = pq_pi_new();
    pi_t nodes[256];
    int nodes_pos = 0;
    int prev[TRACK_MAX], dist[TRACK_MAX];
    for (int i = 0; i < TRACK_MAX; ++i) {
        dist[i] = 1e9;
    }

    train_data_t train_data = init_train_data_a();

    char forbidden_segments[TRACK_SEGMENTS_MAX];
    state_get_forbidden_segments(forbidden_segments);

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

    int speed = (train->speed > 0) ? train->speed : train->old_speed;
    int stopping_distance = train_data.stopping_distance[train->id][speed];
    int reverse_edge_weight = train_data.reverse_edge_weight[train->id];
    int fully_stop_fully_start = train_data.starting_distance[train->id][speed] + train_data.stopping_distance[train->id][speed];

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

            int stop_node_count = 0;
            int prev_node = -1;
            for (int i = 0; i < path.path_length - 1; ++i) {
                if (track[path.nodes[i]].reverse == &track[path.nodes[i + 1]]) {
                    path.stop_dest_nodes[stop_node_count] = i + 1;
                    path.stop_dest_offsets[stop_node_count] = train_data.train_length[train->id];
                    int total_path_distance = 0;
                    if (track[path.nodes[i]].type == NODE_MERGE) {
                        total_path_distance += train_data.train_length[train->id] + REVERSE_OVER_MERGE_OFFSET;
                        path.stop_dest_offsets[stop_node_count] = -REVERSE_OVER_MERGE_OFFSET;
                    }
                    if (prev_node == -1) {
                        total_path_distance += train->cur_offset;
                    }
                    for (int j = i - 1; j > prev_node; --j) {
                        total_path_distance += path.distances[j];
                    }

                    int distance_from_end = stopping_distance;
                    // log("total path distance: %d, fully stop fully start: %d\r\n", total_path_distance, fully_stop_fully_start);
                    if (total_path_distance < fully_stop_fully_start) {
                        distance_from_end = get_short_stop_distance(&train_data, train, total_path_distance);
                    }
                    if (track[path.nodes[i]].type == NODE_MERGE) {
                        distance_from_end -= train_data.train_length[train->id] + REVERSE_OVER_MERGE_OFFSET;
                    }
                    // log("distance from end: %d\r\n", distance_from_end);

                    int j;
                    for (j = i - 1; j > prev_node && distance_from_end >= 0; --j) {
                        distance_from_end -= path.distances[j];
                    }
                    path.stop_nodes[stop_node_count] = path.nodes[j + 1];
                    path.stop_offsets[stop_node_count] = -distance_from_end;
                    stop_node_count++;

                    prev_node = i;
                }
            }
            path.stop_node_count = stop_node_count;

            // if no reverses in path
            int total_path_distance = dist[dest] + node_offset - train->cur_offset;

            for (int i = path.path_length - 2; i >= 0; --i) {
                if (track[path.nodes[i]].reverse == &track[path.nodes[i + 1]]) {
                    if (track[path.nodes[i]].type == NODE_MERGE) {
                        total_path_distance = dist[dest] + node_offset - dist[path.nodes[i + 1]] + REVERSE_OVER_MERGE_OFFSET;
                    } else {
                        total_path_distance = dist[dest] + node_offset - dist[path.nodes[i + 1]] - train_data.train_length[train->id];
                    }
                    break;
                }
            }

            if (total_path_distance < fully_stop_fully_start) {
                stopping_distance = get_short_stop_distance(&train_data, train, total_path_distance);
            }

            // uart_printf(CONSOLE, "total path distance: %d, stopping distance: %d\r\n", total_path_distance, stopping_distance);
            // starting from node BEFORE the dest node, find the node and time offset at which we send stop command
            for (int i = 0; i < path_length; ++i) {
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

            path.dest = dest;
            path.dest_offset = node_offset;
            path.path_distance = dist[dest];
            break;
        }
        if (dist[node] < weight) {
            continue;
        }

        switch (track[node].type) {
        case NODE_BRANCH: {
            int straight_seg = track[node].enters_seg[DIR_STRAIGHT];
            if (straight_seg < 0 || !(straight_seg == forbidden_seg || forbidden_segments[straight_seg])) {
                track_edge straight_edge = track[node].edge[DIR_STRAIGHT];
                int node_straight = get_node_index(track, straight_edge.dest);
                add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_straight, straight_edge.dist);
            }
            int curved_seg = track[node].enters_seg[DIR_CURVED];
            if (curved_seg < 0 || !(curved_seg == forbidden_seg || forbidden_segments[curved_seg])) {
                track_edge curved_edge = track[node].edge[DIR_CURVED];
                int node_curved = get_node_index(track, curved_edge.dest);
                add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_curved, curved_edge.dist);
            }
            break;
        }

        case NODE_SENSOR:
        case NODE_MERGE:
        case NODE_ENTER: {
            // one edge
            int ahead_seg = track[node].enters_seg[DIR_AHEAD];
            if (ahead_seg < 0 || !(ahead_seg == forbidden_seg || forbidden_segments[ahead_seg])) {
                track_edge edge = track[node].edge[DIR_AHEAD];
                int node_ahead = get_node_index(track, edge.dest);
                add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, node_ahead, edge.dist);
            }
            break;
        }

        case NODE_EXIT:
            break;

        default:
            ASSERTF(0, "invalid node: %d, type: %d", node, track[node].type);
        }

        if (track[node].type == NODE_EXIT || track[node].type == NODE_MERGE) {
            int ahead_seg = track[node].enters_seg[DIR_AHEAD];
            if (ahead_seg < 0 || !(ahead_seg == forbidden_seg || forbidden_segments[ahead_seg])) {
                int reverse_node = get_node_index(track, track[node].reverse);
                add_to_queue(&pq, dist, prev, nodes, &nodes_pos, node, reverse_node, reverse_edge_weight);
            }
        }
    }
    // if (path.path_length == 0) printf(CONSOLE, "train %d path impossible!\r\n", train->id);
    // for (int i = 0; i < path.path_length; ++i) {
    //     printf(CONSOLE, "train %d path node %d: %s\r\n", train->id, i, track[path.nodes[i]].name);
    // }
    ASSERTF(path.path_length == 0 || path.nodes[0] == src || path.nodes[0] == reverse_node,
        "new path for train %d starts on node %s, not cur node %s or reverse node %s\r\n",
        train->id, track[path.nodes[0]].name, track[src].name, track[reverse_node].name);
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
