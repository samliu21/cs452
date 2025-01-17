#include "task.h"
#include "rpi.h"

uint64_t n_task = 0;

task_t task_new(priority_t priority, uint64_t sp, func_t entry_point)
{
    task_t task;
    task.tid = n_task++;
    task.elr = (uint64_t)entry_point;
    task.priority = priority;
    task.spsr = 0;
    task.sp = sp;
    return task;
}
