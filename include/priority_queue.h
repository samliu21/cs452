#ifndef _priority_queue_h_
#define _priority_queue_h_

#include "task.h"

typedef struct priority_queue {
    task_t* head;
    task_t* tail;
    int size;
} priority_queue;

#endif