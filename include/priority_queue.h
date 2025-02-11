#ifndef _priority_queue_h_
#define _priority_queue_h_

#include "task.h"

typedef struct priority_queue_t {
    task_t* head;
    task_t* tail;
    int size;
} priority_queue_t;

priority_queue_t pq_new();
void pq_add(priority_queue_t* pq, task_t* task);
task_t* pq_pop(priority_queue_t* pq);
task_t* pq_peek(priority_queue_t* pq);
int pq_empty(priority_queue_t* pq);

#endif
