#include "task.h"
#include "rpi.h"

void task_new(task_t* task, uint64_t tid, uint64_t priority, uint64_t sp, func_t entry_point, task_t* parent_task)
{
    task->tid = tid;
    task->priority = priority;
    task->sp = sp;
    task->elr = (uint64_t)entry_point;
    task->parent_tid = parent_task->tid;
    task->spsr = 0; // this allows interrupts to be enabled on user tasks

    task->next_task = NULL;
    task->senders_queue = queue_new();
    task->state = READY;
    // note: next_slab is set in allocator_new, don't need to default to NULL
}

task_t* get_task(task_t* task_list, uint64_t tid)
{
    task_t* curr_task = task_list;
    while (curr_task && curr_task->tid != tid) {
        curr_task = curr_task->next_task;
    }
    return curr_task;
}
