#include "priority_queue_task.h"
#include "testutils.h"

int _test_pq_task_add()
{
    task_t kernel, t1, t2, t3;
    kernel.tid = 0;
    task_new(&t1, 1, 0, 0, 0, &kernel);
    task_new(&t2, 2, 2, 0, 0, &t1);
    task_new(&t3, 3, 1, 0, 0, &t2);

    priority_queue_task_t pq = pq_task_new();
    TEST_ASSERT(pq.size == 0);
    TEST_ASSERT(pq.head == NULL);
    TEST_ASSERT(pq.tail == NULL);

    pq_task_add(&pq, &t1);
    TEST_ASSERT(pq.size == 1);
    TEST_ASSERT(pq.head == &t1);
    TEST_ASSERT(pq.tail == &t1);
    TEST_ASSERT(t1.parent_tid == 0);

    pq_task_add(&pq, &t2); // add to front
    TEST_ASSERT(pq.size == 2);
    TEST_ASSERT(pq.head == &t2);
    TEST_ASSERT(pq.head->next_task == &t1);
    TEST_ASSERT(pq.tail == &t1);
    TEST_ASSERT(t2.parent_tid == 1);

    pq_task_add(&pq, &t3); // add to middle
    TEST_ASSERT(pq.size == 3);
    TEST_ASSERT(pq.head == &t2);
    TEST_ASSERT(pq.head->next_task == &t3);
    TEST_ASSERT(pq.head->next_task->next_task == &t1);
    TEST_ASSERT(pq.tail == &t1);
    TEST_ASSERT(t3.parent_tid == 2);

    return 1;
}

int _test_pq_task_pop()
{
    task_t kernel, t1, t2, t3;
    kernel.tid = 0;
    task_new(&t1, 1, 0, 0, 0, &kernel);
    task_new(&t2, 2, 2, 0, 0, &t1);
    task_new(&t3, 3, 1, 0, 0, &t2);

    priority_queue_task_t pq = pq_task_new();
    pq_task_add(&pq, &t1);
    pq_task_add(&pq, &t2);
    pq_task_add(&pq, &t3);

    task_t* t;
    t = pq_task_pop(&pq);
    TEST_ASSERT(t == &t2);
    TEST_ASSERT(pq.size == 2);

    t = pq_task_pop(&pq);
    TEST_ASSERT(t == &t3);
    TEST_ASSERT(pq.size == 1);

    t = pq_task_pop(&pq);
    TEST_ASSERT(t == &t1);
    TEST_ASSERT(pq.size == 0);
    return 1;
}

int run_pq_task_tests()
{
    TEST_RUN(_test_pq_task_add);
    TEST_RUN(_test_pq_task_pop);
    return 1;
}
