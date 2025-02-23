#include "priority_queue_pi.h"
#include "rpi.h"
#include <stdlib.h>

priority_queue_pi_t pq_pi_new()
{
    priority_queue_pi_t pq;
    pq.head = NULL;
    pq.tail = NULL;
    pq.size = 0;
    return pq;
}

void pq_pi_add(priority_queue_pi_t* pq, pi_t* pi)
{
    if (pq->size == 0) {
        pq->head = pi;
        pq->tail = pi;
        pq->size = 1;
        return;
    }

    pq->size++;
    pi_t* t = pq->head;
    pi_t* pv = NULL;
    while (t && pi->weight >= t->weight) {
        pv = t;
        t = t->next;
    }

    // after the while loop, t is the first task with lower priority than task (or NULL if no such task exists)
    // pv is the task right before t (or NULL if t is the head of the queue)
    if (pv) {
        pv->next = pi;
        pi->next = t;
    } else {
        pi->next = pq->head;
        pq->head = pi;
    }
}

pi_t* pq_pi_pop(priority_queue_pi_t* pq)
{
    ASSERT(pq->size > 0, "pop from empty pi priority queue");
    pq->size--;

    pi_t* t = pq->head;
    pq->head = pq->head->next;
    if (pq->size == 0) {
        pq->tail = NULL;
    }
    t->next = NULL;
    return t;
}

pi_t* pq_pi_peek(priority_queue_pi_t* pq)
{
    ASSERT(pq->size > 0, "peek from empty pi priority queue");
    return pq->head;
}

int pq_pi_empty(priority_queue_pi_t* pq)
{
    return pq->size == 0;
}
