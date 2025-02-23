#ifndef _priority_queue_h_
#define _priority_queue_h_

#include "task.h"

typedef struct priority_queue_task_t {
    task_t* head;
    task_t* tail;
    int size;
} priority_queue_task_t;

priority_queue_task_t pq_task_new();
void pq_task_add(priority_queue_task_t* pq, task_t* task);
task_t* pq_task_pop(priority_queue_task_t* pq);
task_t* pq_task_peek(priority_queue_task_t* pq);
int pq_task_empty(priority_queue_task_t* pq);

#endif
