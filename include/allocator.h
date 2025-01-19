#ifndef _slab_h_
#define _slab_h_

#include "common.h"
#include "task.h"

typedef struct allocator_t {
    task_t* slabs;
    uint64_t n_slabs;
    uint64_t n_free;
    uint64_t n_alloc;
    task_t* free_list;
    task_t* alloc_list;
} allocator_t;

allocator_t allocator_new(task_t* slabs, uint64_t n_slabs);
task_t* allocator_alloc(allocator_t* allocator);
void allocator_free(allocator_t* allocator, task_t* task);

#endif
