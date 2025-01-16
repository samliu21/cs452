#ifndef _task_h_
#define _task_h_ 1

#include "common.h"

typedef enum priority_t {
    undefined = -1,
    priority_0 = 0,
    priority_1 = 1,
    priority_2 = 2,
} priority_t;

typedef struct task_t {
    uint64_t registers[31];
    uint64_t elr;
    uint64_t sp;
    uint64_t spsr;

    priority_t priority;
    uint64_t tid;
    uint64_t parent_tid;
    struct task_t* next_task;
    struct task_t* next_slab;
} task_t;

extern int myinc(int i);

task_t task_new(priority_t priority, uint64_t sp, func_t entry_point);
void task_run(task_t task);

#endif /* task.h */