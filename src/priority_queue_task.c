#include "priority_queue_task.h"
#include "rpi.h"
#include <stdlib.h>

priority_queue_task_t pq_task_new()
{
    priority_queue_task_t pq;
    pq.head = NULL;
    pq.tail = NULL;
    pq.size = 0;
    return pq;
}

void pq_task_add(priority_queue_task_t* pq, task_t* task)
{
    if (pq->size == 0) {
        pq->head = task;
        pq->tail = task;
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

task_t* pq_task_pop(priority_queue_task_t* pq)
{
    ASSERT(pq->size > 0, "pop from empty task priority queue");
    pq->size--;

    task_t* t = pq->head;
    pq->head = pq->head->next_task;
    if (pq->size == 0) {
        pq->tail = NULL;
    }
    t->next_task = NULL;
    return t;
}

task_t* pq_task_peek(priority_queue_task_t* pq)
{
    ASSERT(pq->size > 0, "peek from empty task priority queue");
    return pq->head;
}

int pq_task_empty(priority_queue_task_t* pq)
{
    return pq->size == 0;
}
