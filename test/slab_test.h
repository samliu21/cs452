#include "slab.h"
#include "testutils.h"

int test_allocator()
{
    task_t nodes[4];
    allocator_t a = allocator_new(nodes, 2);
    TEST_ASSERT(a.n_alloc == 0 && a.n_free == 2);

    task_t* t1 = allocator_alloc(&a);
    task_t* t2 = allocator_alloc(&a);
    TEST_ASSERT(a.n_alloc == 2 && a.n_free == 0);

    allocator_free(&a, t1);
    TEST_ASSERT(a.n_alloc == 1 && a.n_free == 1);
    allocator_free(&a, t2);
    TEST_ASSERT(a.n_alloc == 0 && a.n_free == 2);

    return 1;
}

void slab_tests()
{
    TEST_RUN(test_allocator);
}
