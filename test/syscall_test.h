#ifndef _syscall_test_h_
#define _syscall_test_h_

#include "allocator.h"
#include "syscall_asm.h"
#include "syscall_handler.h"
#include "testutils.h"

#define SYSCALL_TEST_NUM_TASKS 2

void task2func()
{
    exit();
}

void task1func()
{
    uint64_t tid = my_tid();
    TEST_TASK_ASSERT(tid == 500);

    uint64_t parent_tid = my_parent_tid();
    TEST_TASK_ASSERT(parent_tid == 0);

    uint64_t new_tid = create(2, &task2func);
    TEST_TASK_ASSERT(new_tid == 501);

    yield();

    exit();
}

int _test_syscall()
{
    task_t kernel_task;
    kernel_task.tid = 0;

    task_t tasks[SYSCALL_TEST_NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, SYSCALL_TEST_NUM_TASKS);
    char stack[SYSCALL_TEST_NUM_TASKS * TEST_STACK_SIZE];

    uint64_t n_tasks = 500;
    task_t* task1 = allocator_new_task(&allocator, stack, n_tasks++, 1, &task1func, &kernel_task);

    priority_queue_t scheduler;

    main_context_t context;
    context.kernel_task = &kernel_task;
    context.allocator = &allocator;
    context.stack = stack;
    context.n_tasks = &n_tasks;
    context.scheduler = &scheduler;
    context.active_task = task1;

    uint64_t syndrome;

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_MYTID);
    my_tid_handler(&context);

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_MYPARENTTID);
    my_parent_tid_handler(&context);

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_CREATE);
    TEST_ASSERT(task1->registers[0] == 2);
    TEST_ASSERT(task1->registers[1] == (uint64_t)&task2func);
    create_handler(&context);
    task_t* task2 = pq_pop(&scheduler);
    TEST_ASSERT(task2->priority == 2);
    TEST_ASSERT(task2->elr == (uint64_t)&task2func);

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_EXIT);
    exit_handler(&context);

    syndrome = enter_task(&kernel_task, task2) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_EXIT);
    return 1;
}

void run_syscall_tests()
{
    TEST_RUN(_test_syscall);
}

#endif /* syscall_test.h */
