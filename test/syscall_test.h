#ifndef _syscall_test_h_
#define _syscall_test_h_

#include "allocator.h"
#include "syscall_asm.h"
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
    TEST_TASK_ASSERT(new_tid == 1000);

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

    task_t* task1 = allocator_new_task(&allocator, stack, 500, 1, &task1func, &kernel_task);

    uint64_t syndrome;

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_MYTID);
    task1->registers[0] = task1->tid;

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_MYPARENTTID);
    task1->registers[0] = task1->parent_tid;

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_CREATE);
    TEST_ASSERT(task1->registers[0] == 2);
    TEST_ASSERT(task1->registers[1] == (uint64_t)&task2func);
    task_t* task2 = allocator_new_task(&allocator, stack, 1000, 2, &task2func, task1);
    task1->registers[0] = task2->tid;

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_EXIT);

    syndrome = enter_task(&kernel_task, task2) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_EXIT);
    return 1;
}

void run_syscall_tests()
{
    TEST_RUN(_test_syscall);
}

#endif /* syscall_test.h */
