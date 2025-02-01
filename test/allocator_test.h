#include "allocator.h"
#include "testutils.h"

int _test_allocator()
{
    task_t nodes[4];
    allocator_t a = allocator_new(nodes, 2);
    TEST_ASSERT(a.n_alloc == 0 && a.n_free == 2);

    task_t* t1 = allocator_alloc(&a);
    TEST_ASSERT(a.n_alloc == 1 && a.n_free == 1);
    task_t* t2 = allocator_alloc(&a);
    TEST_ASSERT(a.n_alloc == 2 && a.n_free == 0);

    allocator_free(&a, t1);
    TEST_ASSERT(a.n_alloc == 1 && a.n_free == 1);
    allocator_free(&a, t2);
    TEST_ASSERT(a.n_alloc == 0 && a.n_free == 2);

    task_t* t3 = allocator_alloc(&a);
    TEST_ASSERT(a.n_alloc == 1 && a.n_free == 1);
    task_t* t4 = allocator_alloc(&a);
    TEST_ASSERT(a.n_alloc == 2 && a.n_free == 0);

    allocator_free(&a, t3);
    TEST_ASSERT(a.n_alloc == 1 && a.n_free == 1);
    allocator_free(&a, t4);
    TEST_ASSERT(a.n_alloc == 0 && a.n_free == 2);

    return 1;
}

int run_allocator_tests()
{
    TEST_RUN(_test_allocator);
	return 1;
}
