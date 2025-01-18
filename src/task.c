#include "task.h"
#include "rpi.h"

task_t task_new(priority_t priority, uint64_t sp, func_t entry_point)
{
    task_t task;
    task.tid = 5;
    task.elr = (uint64_t)entry_point;
    task.priority = priority;
    task.spsr = 0;
    task.sp = sp;
    return task;
}
