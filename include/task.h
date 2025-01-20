#ifndef _task_h_
#define _task_h_

#include "common.h"
#include "stack.h"

// TODO: turn this into a uint64_t
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

// TODO: for all our asm interfaces, can we pass in pointer and have asm auto-cast to uint64_t address?
extern uint64_t enter_task(uint64_t kernel_task, uint64_t task);

void task_new(task_t* task, uint64_t tid, priority_t priority, uint64_t sp, func_t entry_point, task_t* parent_task);

#endif /* task.h */