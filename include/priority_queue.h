#ifndef _priority_queue_h_
#define _priority_queue_h_

#include "task.h"

typedef struct priority_queue {
    task_t* head;
    task_t* tail;
    int size;
} priority_queue;

priority_queue pq_new();
void pq_add(priority_queue* pq, task_t* task);
task_t* pq_pop(priority_queue* pq);

#endif
