#ifndef _task_h_
#define _task_h_

#include "common.h"
#include "queue.h"

typedef enum {
    READY,
    SENDWAIT,
    RECEIVEWAIT,
    REPLYWAIT,
    EVENTWAIT,
} state_t;

typedef struct task_t {
    uint64_t registers[31];
    uint64_t elr;
    uint64_t sp;
    uint64_t spsr;

    uint64_t priority;
    uint64_t tid;
    uint64_t parent_tid;
    struct task_t* next_task;
    struct task_t* next_slab;
    queue_t senders_queue;
    state_t state;
} task_t;

extern uint64_t enter_task(task_t* kernel_task, task_t* task);

void task_new(task_t* task, uint64_t tid, uint64_t priority, uint64_t sp, func_t entry_point, task_t* parent_task);

task_t* get_task(task_t* task_list, uint64_t tid);

#endif /* task.h */