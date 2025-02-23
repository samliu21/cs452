#include "priority_queue_pi.h"
#include "track_data.h"
#include "track_node.h"
#include <stdlib.h>

pi_t* dijkstra(track_node* track, int src)
{
    priority_queue_pi_t pq = pq_pi_new();
    pi_t nodes[TRACK_MAX];
    int prev[TRACK_MAX];
    for (int i = 0; i < TRACK_MAX; ++i) {
        nodes[i].f = (i == src) ? 0 : __INT32_MAX__;
        nodes[i].s = i;
        prev[i] = -1;
        pq_pi_add(&pq, nodes + i);
    }

    while (!pq_pi_empty(&pq)) {
        pi_t* node = pq_pi_pop(&pq);

        switch (track[node->s].type) {
        case NODE_BRANCH:
            // two edges
            break;

        case NODE_SENSOR:
        case NODE_MERGE:
        case NODE_ENTER:
            // one edge
            break;

        case NODE_EXIT:
            break;

        default:
            ASSERTF(0, "Invalid Node");
        }
    }
    return NULL;
}