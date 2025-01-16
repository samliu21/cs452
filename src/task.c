#include "task.h"
#include "rpi.h"

task_t task_new(priority_t priority, uint64_t sp, func_t entry_point)
{
    task_t task;
    task.elr = (uint64_t)entry_point;
    task.priority = priority;
    task.spsr = 0;
    task.sp = sp;
    return task;
}

void task_run(task_t *kernel_task, task_t *task)
{
    enter_task((uint64_t) kernel_task, (uint64_t) task);
}
