#include "priority_queue.h"
#include "testutils.h"

int test_pq_add()
{
    task_t t1, t2, t3;
    task_new(&t1, priority_0, 0, 0);
    task_new(&t2, priority_2, 0, 0);
    task_new(&t3, priority_1, 0, 0);

    priority_queue_t pq = pq_new();
    TEST_ASSERT(pq.size == 0);
    TEST_ASSERT(pq.head == NULL);
    TEST_ASSERT(pq.tail == NULL);

    pq_add(&pq, &t1);
    TEST_ASSERT(pq.size == 1);
    TEST_ASSERT(pq.head == &t1);
    TEST_ASSERT(pq.tail == &t1);

    pq_add(&pq, &t2); // add to front
    TEST_ASSERT(pq.size == 2);
    TEST_ASSERT(pq.head == &t2);
    TEST_ASSERT(pq.head->next_task == &t1);
    TEST_ASSERT(pq.tail == &t1);

    pq_add(&pq, &t3); // add to middle
    TEST_ASSERT(pq.size == 3);
    TEST_ASSERT(pq.head == &t2);
    TEST_ASSERT(pq.head->next_task == &t3);
    TEST_ASSERT(pq.head->next_task->next_task == &t1);
    TEST_ASSERT(pq.tail == &t1);

    return 1;
}

int test_pq_pop()
{
    task_t t1, t2, t3;
    task_new(&t1, priority_0, 0, 0);
    task_new(&t2, priority_2, 0, 0);
    task_new(&t3, priority_1, 0, 0);

    priority_queue_t pq = pq_new();
    pq_add(&pq, &t1);
    pq_add(&pq, &t2);
    pq_add(&pq, &t3);

    task_t* t;
    t = pq_pop(&pq);
    TEST_ASSERT(t == &t2);
    TEST_ASSERT(pq.size == 2);

    t = pq_pop(&pq);
    TEST_ASSERT(t == &t3);
    TEST_ASSERT(pq.size == 1);

    t = pq_pop(&pq);
    TEST_ASSERT(t == &t1);
    TEST_ASSERT(pq.size == 0);
    return 1;
}

void pq_tests()
{
    TEST_RUN(test_pq_add);
}
