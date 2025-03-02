#include "queue_task.h"
#include "rpi.h"
#include <stdlib.h>

#include "task.h"

queue_task_t queue_task_new()
{
    queue_task_t q;
    q.head = NULL;
    q.tail = NULL;
    q.size = 0;
    return q;
}

void queue_task_add(queue_task_t* q, task_t* task)
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

task_t* queue_task_pop(queue_task_t* q)
{
    ASSERT(q->size > 0, "pop from empty queue");
    q->size--;

    task_t* t = q->head;
    q->head = q->head->next_task;
    if (q->size == 0) {
        q->head = NULL;
        q->tail = NULL;
    }
    t->next_task = NULL;
    return t;
}

void queue_task_delete(queue_task_t* q, task_t* task)
{
    task_t* t = q->head;
    if (t == task) {
        queue_task_pop(q);
        return;
    }
    q->size--;
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
    task->next_task = NULL;
    if (q->size == 0) {
        q->head = NULL;
        q->tail = NULL;
    }
}

task_t* queue_task_peek(queue_task_t* q)
{
    ASSERT(q->size > 0, "peek from empty queue");
    return q->head;
}

int queue_task_empty(queue_task_t* q)
{
    return q->size == 0;
}
