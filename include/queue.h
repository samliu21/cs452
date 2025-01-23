#ifndef _queue_h_
#define _queue_h_

typedef struct task_t task_t; // forward declare

typedef struct queue_t {
    task_t* head;
    task_t* tail;
    int size;
} queue_t;

queue_t queue_new();
void queue_add(queue_t* q, task_t* task);
task_t* queue_pop(queue_t* q);
task_t* queue_peek(queue_t* q);
int queue_empty(queue_t* q);

#endif
