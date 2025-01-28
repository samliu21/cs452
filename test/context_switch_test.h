#ifndef _context_switch_test_h_
#define _context_switch_test_h_

#include "allocator.h"
#include "syscall_asm.h"
#include "testutils.h"

#define CONTEXT_SWITCH_TEST_NUM_TASKS 1

void taskfunc()
{
    // set user registers to 200, 201, ..., and ensure they stay the same after a context switch
    uint64_t registers[31];
    debug_set_registers(200);
    yield();
    debug_dump_registers(registers);
    for (uint64_t i = 2; i < 19; ++i) { // goes up to 18 because 19 and above are callee-saved!!!
        TEST_TASK_ASSERT(registers[i] == 200 + i);
    }
    yield();
}

int _test_context_switch()
{
    task_t kernel_task;
    kernel_task.tid = 0;

    task_t tasks[CONTEXT_SWITCH_TEST_NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, CONTEXT_SWITCH_TEST_NUM_TASKS);
    char stack[CONTEXT_SWITCH_TEST_NUM_TASKS * TEST_STACK_SIZE];

    task_t* task = allocator_new_task(&allocator, stack, 1, 1, &taskfunc, &kernel_task);
    uint64_t syndrome;
    uint64_t registers[31];

    // set kernel registers to 100, 101, ..., and ensure they stay the same after a context switch
    debug_set_registers(100);
    enter_task(&kernel_task, task);

    debug_dump_registers(registers);
    for (uint64_t i = 2; i < 19; ++i) {
        TEST_ASSERT(registers[i] == 100 + i);
    }

    debug_set_registers(100);
    syndrome = enter_task(&kernel_task, task) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);

    task->registers[0] = task->parent_tid;
    return 1;
}

void run_context_switch_tests()
{
    TEST_RUN(_test_context_switch);
}

#endif
