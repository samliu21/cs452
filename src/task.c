#include "task.h"
#include "rpi.h"

void task_new(task_t* task, uint64_t tid, uint64_t priority, uint64_t sp, func_t entry_point, task_t* parent_task)
{
    task->tid = tid;
    task->priority = priority;
    task->sp = sp;
    task->elr = (uint64_t)entry_point;
    task->parent_tid = parent_task->tid;
    task->spsr = 0;
}
