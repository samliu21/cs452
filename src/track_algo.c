#include "priority_queue_pi.h"
#include "rpi.h"
#include "track_data.h"
#include "track_node.h"
#include <stdlib.h>

int get_node_index(track_node* track_begin, track_node* node)
{
    return node - track_begin;
}

void add_to_queue(priority_queue_pi_t* pq, int* dist, int* prev, pi_t* nodes, int* nodes_pos, int prev_node, int cur_node, int weight)
{
    // int node_straight = get_node_index(track, track[node].edge[DIR_STRAIGHT].dest);
    int dist_cand = dist[prev_node] + weight;
    if (dist_cand < dist[cur_node]) {
        nodes[*nodes_pos].weight = dist_cand;
        nodes[*nodes_pos].id = cur_node;
        pq_pi_add(pq, &nodes[*nodes_pos++]);
        dist[cur_node] = dist_cand;
        prev[cur_node] = prev_node;
    }
}

void get_shortest_path(track_node* track, int src, int dest)
{
    priority_queue_pi_t pq = pq_pi_new();
    pi_t nodes[2000];
    int nodes_pos = 0;
    int prev[TRACK_MAX], dist[TRACK_MAX];
    memset(dist, __INT_MAX__, TRACK_MAX);

    nodes[nodes_pos].weight = 0;
    prev[nodes_pos] = -1;
    pq_pi_add(&pq, &nodes[nodes_pos++]);
    dist[src] = 0;
    prev[src] = -1;

    while (!pq_pi_empty(&pq)) {
        pi_t* pi = pq_pi_pop(&pq);
        int node = pi->id;
        if (node == dest) {
            int n = dest;
            while (n != -1) {
                uart_printf(CONSOLE, "%d ", n);
                n = prev[n];
            }
            break;
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
}