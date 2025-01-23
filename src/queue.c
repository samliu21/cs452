#include "queue.h"
#include "rpi.h"
#include <stdlib.h>

#include "task.h"

queue_t queue_new()
{
    queue_t q;
    q.head = NULL;
    q.tail = NULL;
    q.size = 0;
    return q;
}

void queue_add(queue_t* q, task_t* task)
{
    if (q->size == 0) {
        q->head = task;
        q->tail = task;
    } else {
        q->tail->next_task = task;
        q->tail = task;
    }
    q->size++;
}

task_t* queue_pop(queue_t* q)
{
    ASSERT(q->size > 0, "pop from empty queue");
    q->size--;

    task_t* t = q->head;
    q->head = q->head->next_task;
    if (q->size == 0) {
        q->tail = NULL;
    }
    return t;
}

void queue_delete(queue_t* q, task_t* task)
{
    q->size--;
    task_t* t = q->head;
    if (t == task) {
        queue_pop(q);
        return;
    }
    while (t->next_task && t->next_task != task) {
        t = t->next_task;
    }
    ASSERT(t->next_task != NULL, "task not found in queue");
    if (t->next_task) {
        if (!task->next_task) {
            q->tail = t;
        }
        t->next_task = task->next_task;
    }
}

task_t* queue_peek(queue_t* q)
{
    ASSERT(q->size > 0, "peek from empty queue");
    return q->head;
}

int queue_empty(queue_t* q)
{
    return q->size == 0;
}
