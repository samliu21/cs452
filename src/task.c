#include "task.h"
#include "rpi.h"

void task_new(task_t* task, priority_t priority, uint64_t sp, func_t entry_point)
{
    task->tid = 5;
    task->elr = (uint64_t)entry_point;
    task->priority = priority;
    task->spsr = 0;
    task->sp = sp;
}
