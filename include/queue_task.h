#ifndef _queue_h_
#define _queue_h_

typedef struct task_t task_t; // forward declare

typedef struct queue_task_t {
    task_t* head;
    task_t* tail;
    int size;
} queue_task_t;

queue_task_t queue_task_new();
void queue_task_add(queue_task_t* q, task_t* task);
task_t* queue_task_pop(queue_task_t* q);
void queue_task_delete(queue_task_t* q, task_t* task);
task_t* queue_task_peek(queue_task_t* q);
int queue_task_empty(queue_task_t* q);

#endif
