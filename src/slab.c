#include "slab.h"
#include "rpi.h"
#include <stdlib.h>

allocator_t allocator_new(task_t* slabs, uint64_t n_slabs)
{
    allocator_t allocator;
    allocator.slabs = slabs;
    allocator.n_slabs = n_slabs;
    allocator.n_free = n_slabs;
    allocator.n_alloc = 0;
    for (uint64_t i = 0; i < n_slabs - 1; ++i) {
        slabs[i].next_slab = &slabs[i + 1];
    }
    allocator.free_list = slabs;
    allocator.alloc_list = NULL;
    return allocator;
}

task_t* allocator_alloc(allocator_t* allocator)
{
    ASSERT(allocator->n_free > 0, "no free slabs available");
    task_t* task = allocator->free_list;
    // update free list
    allocator->free_list = task->next_slab;
    allocator->n_free--;

    // update alloc list
    task->next_slab = allocator->alloc_list;
    allocator->alloc_list = task;
    allocator->n_alloc++;

    return task;
}

void allocator_free(allocator_t* allocator, task_t* task)
{
    task_t* t = allocator->alloc_list;
    task_t* pv = NULL;
    while (t) {
        if (t == task) {
            // updated alloc list
            if (pv) {
                pv->next_slab = t->next_slab;
            } else {
                allocator->alloc_list = t->next_slab;
            }
            allocator->n_alloc--;

            // update free list
            t->next_slab = allocator->free_list;
            allocator->free_list = t;
            allocator->n_free++;

            return;
        }
        pv = t;
        t = t->next_slab;
    }
    ASSERT(0, "attempted to free a task that was not allocated");
}
