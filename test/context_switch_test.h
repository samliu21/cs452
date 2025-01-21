#include "allocator.h"
#include "syscall.h"
#include "testutils.h"

#define TEST_STACK_SIZE 1000

void taskfunc()
{
    uint64_t registers[31];
    debug_set_registers(200);
    yield();
    debug_dump_registers(registers);
    for (int i = 1; i < 29; ++i) {
        TEST_TASK_ASSERT(registers[i] == 200 + i);
    }
    yield();
}

int _test_context_switch()
{
    task_t kernel_task;
    kernel_task.tid = 0;

    task_t tasks[1];
    allocator_t allocator = allocator_new(tasks, NUM_TASKS);
    char stack[TEST_STACK_SIZE];
    task_t* task = allocator_new_task(&allocator, stack, 1, 1, &taskfunc, &kernel_task);
    uint64_t syndrome;
    uint64_t registers[31];

    debug_set_registers(100);
    enter_task(&kernel_task, task);

    debug_dump_registers(registers);
    for (int i = 1; i < 29; ++i) {
        TEST_ASSERT(registers[i] == 100 + i);
    }

    debug_set_registers(100);
    syndrome = enter_task(&kernel_task, task) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);

    task->registers[0] = task->parent_tid;
}

void run_context_switch_tests()
{
    TEST_RUN(_test_context_switch);
}
