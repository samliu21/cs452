#include "queue_pi.h"
#include "rpi.h"
#include <stdlib.h>

queue_pi_t queue_pi_new()
{
    queue_pi_t q;
    q.head = NULL;
    q.tail = NULL;
    q.size = 0;
    return q;
}

void queue_pi_add(queue_pi_t* q, pi_t* pi)
{
    if (q->size == 0) {
        q->head = pi;
        q->tail = pi;
    } else {
        q->tail->next = pi;
        q->tail = pi;
    }
    q->size++;
}

pi_t* queue_pi_pop(queue_pi_t* q)
{
    ASSERT(q->size > 0, "pop from empty queue");
    q->size--;

    pi_t* t = q->head;
    q->head = q->head->next;
    if (q->size == 0) {
        q->head = NULL;
        q->tail = NULL;
    }
    t->next = NULL;
    return t;
}

void queue_pi_delete(queue_pi_t* q, pi_t* pi)
{
    pi_t* t = q->head;
    if (t == pi) {
        queue_pi_pop(q);
        return;
    }
    q->size--;
    while (t->next && t->next != pi) {
        t = t->next;
    }
    ASSERT(t->next != NULL, "pi not found in queue");
    if (t->next) {
        if (!pi->next) {
            q->tail = t;
        }
        t->next = pi->next;
    }
    pi->next = NULL;
    if (q->size == 0) {
        q->head = NULL;
        q->tail = NULL;
    }
}

pi_t* queue_pi_peek(queue_pi_t* q)
{
    ASSERT(q->size > 0, "peek from empty queue");
    return q->head;
}

int queue_pi_empty(queue_pi_t* q)
{
    return q->size == 0;
}
