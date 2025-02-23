#include "priority_queue_pi.h"
#include "testutils.h"

int _test_pq_pi_add()
{
    pi_t p1, p2, p3;
    p1.weight = 7;
    p1.next = NULL;
    p2.weight = 3;
    p2.next = NULL;
    p3.weight = 5;
    p3.next = NULL;

    priority_queue_pi_t pq = pq_pi_new();
    TEST_ASSERT(pq.size == 0);
    TEST_ASSERT(pq.head == NULL);
    TEST_ASSERT(pq.tail == NULL);

    pq_pi_add(&pq, &p1);
    TEST_ASSERT(pq.size == 1);
    TEST_ASSERT(pq.head == &p1);
    TEST_ASSERT(pq.tail == &p1);

    pq_pi_add(&pq, &p2); // add to front
    TEST_ASSERT(pq.size == 2);
    TEST_ASSERT(pq.head == &p2);
    TEST_ASSERT(pq.head->next == &p1);
    TEST_ASSERT(pq.tail == &p1);

    pq_pi_add(&pq, &p3); // add to middle
    TEST_ASSERT(pq.size == 3);
    TEST_ASSERT(pq.head == &p2);
    TEST_ASSERT(pq.head->next == &p3);
    TEST_ASSERT(pq.head->next->next == &p1);
    TEST_ASSERT(pq.tail == &p1);

    return 1;
}

int _test_pq_pi_pop()
{
    pi_t p1, p2, p3;
    p1.weight = 7;
    p1.next = NULL;
    p2.weight = 3;
    p2.next = NULL;
    p3.weight = 5;
    p3.next = NULL;

    priority_queue_pi_t pq = pq_pi_new();
    pq_pi_add(&pq, &p1);
    pq_pi_add(&pq, &p2);
    pq_pi_add(&pq, &p3);

    pi_t* t;
    t = pq_pi_pop(&pq);
    TEST_ASSERT(t == &p2);
    TEST_ASSERT(pq.size == 2);

    t = pq_pi_pop(&pq);
    TEST_ASSERT(t == &p3);
    TEST_ASSERT(pq.size == 1);

    t = pq_pi_pop(&pq);
    TEST_ASSERT(t == &p1);
    TEST_ASSERT(pq.size == 0);
    return 1;
}

int run_pq_pi_tests()
{
    TEST_RUN(_test_pq_pi_add);
    TEST_RUN(_test_pq_pi_pop);
    return 1;
}
