#include "priority_queue.h"
#include "rpi.h"
#include <stdlib.h>

priority_queue pq_new()
{
    priority_queue pq;
    pq.head = NULL;
    pq.tail = NULL;
    pq.size = 0;
    return pq;
}

void pq_add(priority_queue* pq, task_t* task)
{
    if (pq->size == 0) {
        pq->head = task;
        pq->tail = task;
        task->next_task = NULL;
        pq->size = 1;
        return;
    }

    pq->size++;
    task_t* t = pq->head;
    task_t* pv = NULL;
    while (t && task->priority <= t->priority) {
        pv = t;
        t = t->next_task;
    }

    // after the while loop, t is the first task with lower priority than task (or NULL if no such task exists)
    // pv is the task right before t (or NULL if t is the head of the queue)
    if (pv) {
        pv->next_task = task;
        task->next_task = t;
    } else {
        task->next_task = pq->head;
        pq->head = task;
    }
}

task_t* pq_pop(priority_queue* pq)
{
    ASSERT(pq->size > 0, "pop from empty priority queue");
    pq->size--;

    task_t* t = pq->head;
    pq->head = pq->head->next_task;
    if (pq->size == 0) {
        pq->tail = NULL;
    }
    return t;
}
