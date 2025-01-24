#ifndef _queue_test_h_
#define _queue_test_h_

#include "queue.h"
#include "testutils.h"

int _test_queue()
{
    task_t kernel, t1, t2, t3;
    kernel.tid = 0;
    task_new(&t1, 1, 0, 0, 0, &kernel);
    task_new(&t2, 2, 2, 0, 0, &t1);
    task_new(&t3, 3, 1, 0, 0, &t2);

    queue_t q = queue_new();
    TEST_ASSERT(q.size == 0);
    TEST_ASSERT(q.head == NULL);
    TEST_ASSERT(q.tail == NULL);

    queue_add(&q, &t1);
    TEST_ASSERT(q.size == 1);
    TEST_ASSERT(q.head == &t1);

    queue_add(&q, &t2);
    TEST_ASSERT(q.size == 2);
    TEST_ASSERT(q.head == &t1);
    TEST_ASSERT(q.head->next_task == &t2);
    TEST_ASSERT(q.tail == &t2);

    queue_add(&q, &t3);
    TEST_ASSERT(q.size == 3);
    TEST_ASSERT(q.head == &t1);
    TEST_ASSERT(q.head->next_task == &t2);
    TEST_ASSERT(q.head->next_task->next_task == &t3);
    TEST_ASSERT(q.tail == &t3);

    queue_pop(&q);
    TEST_ASSERT(q.size == 2);
    TEST_ASSERT(q.head == &t2);
    TEST_ASSERT(q.tail == &t3);

    return 1;
}

int _test_queue_delete()
{
    task_t kernel, t1, t2, t3;
    kernel.tid = 0;
    task_new(&t1, 1, 0, 0, 0, &kernel);
    task_new(&t2, 2, 2, 0, 0, &t1);
    task_new(&t3, 3, 1, 0, 0, &t2);

    queue_t q = queue_new();
    queue_add(&q, &t1);
    queue_add(&q, &t2);
    queue_add(&q, &t3);

    queue_delete(&q, &t2);
    TEST_ASSERT(q.size == 2);
    TEST_ASSERT(q.head == &t1);
    TEST_ASSERT(q.head->next_task == &t3);
    TEST_ASSERT(q.tail == &t3);

    queue_delete(&q, &t3);
    TEST_ASSERT(q.size == 1);
    TEST_ASSERT(q.head == &t1);
    TEST_ASSERT(q.tail == &t1);

    return 1;
}

void run_queue_tests()
{
    // TODO: make these tests WAY more comprehensive
    TEST_RUN(_test_queue);
    TEST_RUN(_test_queue_delete);
}

#endif