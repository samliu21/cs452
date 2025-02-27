#include "track_algo.h"
#include "priority_queue_pi.h"
#include "rpi.h"
#include "track_data.h"
#include "track_node.h"
#include "train.h"
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

track_path_t get_shortest_path(track_node* track, int src, int dest)
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

            // starting from node BEFORE the dest node, find the node and time offset at which we send stop command
            for (int i = 1; i < path_length; ++i) {
                int cur_node = path_reverse[i];
                int distance_from_end = dist[dest] - dist[cur_node];
                if (track[cur_node].type == NODE_SENSOR && distance_from_end >= TRAIN_STOPPING_DISTANCE) {
                    path.stop_node = cur_node;
                    path.stop_time_offset = (distance_from_end - TRAIN_STOPPING_DISTANCE) * 1000 / TRAIN_SPEED;
                    break;
                }
                if (i == path_length - 1) {
                    ASSERT(0, "could not find stop node");
                }
            }

            for (int i = path_length - 1; i >= 0; --i) {
                track_path_add(&path, path_reverse[i]);
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

reachable_sensors_t reachable_sensors_new()
{
    reachable_sensors_t rs;
    rs.size = 0;
    return rs;
}

void reachable_sensors_add(reachable_sensors_t* rs, int sensor, int distance)
{
    rs->sensors[rs->size] = sensor;
    rs->distances[rs->size] = distance;
    rs->size++;
}

reachable_sensors_t get_reachable_sensors(track_node* track, int src_sensor)
{
    priority_queue_pi_t pq = pq_pi_new();
    pi_t nodes[2000];
    int nodes_pos = 0;
    int prev[TRACK_MAX], dist[TRACK_MAX];
    for (int i = 0; i < TRACK_MAX; ++i) {
        dist[i] = 1e9;
    }

    nodes[nodes_pos].weight = 0;
    nodes[nodes_pos].id = src_sensor;
    pq_pi_add(&pq, &nodes[nodes_pos++]);
    dist[src_sensor] = 0;
    prev[src_sensor] = -1;

    reachable_sensors_t reachable_sensors = reachable_sensors_new();

    while (!pq_pi_empty(&pq)) {
        pi_t* pi = pq_pi_pop(&pq);
        int node = pi->id;
        if (track[node].type == NODE_SENSOR && node != src_sensor) {
            reachable_sensors_add(&reachable_sensors, node, dist[node]);
            continue;
        }
        if (dist[node] < pi->weight) {
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
            ASSERTF(0, "Invalid Node");
        }
    }
    return reachable_sensors;
}
