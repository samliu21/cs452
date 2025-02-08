#ifndef _clock_test_h_
#define _clock_test_h_

#include "allocator.h"
#include "clock_notifier.h"
#include "clock_server.h"
#include "interrupt.h"
#include "name_server.h"
#include "syscall_asm.h"
#include "syscall_handler.h"
#include "testutils.h"

#define CLOCK_TEST_NUM_TASKS 5

void idletaskfunc()
{
    for (;;)
        ;
}

void clock_task1func()
{
    int64_t invalid_input = time(50);
    TEST_TASK_ASSERT(invalid_input == -1);

    uint32_t real_start_time = timer_get_ticks();
    uint64_t clock_start_time = time(3);
    int diff = (int)(real_start_time - clock_start_time);

    invalid_input = delay(3, -50);
    TEST_TASK_ASSERT(invalid_input == -2);

    delay(3, 10);
    uint32_t real_time = timer_get_ticks();
    uint64_t clock_time = time(3);
    TEST_TASK_ASSERT((int)(real_time - clock_time) - diff <= 1);
    TEST_TASK_ASSERT((int)(real_time - clock_time) - diff >= 0);
    TEST_TASK_ASSERT(clock_time == clock_start_time + 10);

    invalid_input = delay_until(3, 5);
    TEST_TASK_ASSERT(invalid_input == -2);

    delay_until(3, clock_start_time + 20);
    real_time = timer_get_ticks();
    clock_time = time(3);
    TEST_TASK_ASSERT((int)(real_time - clock_time) - diff <= 1);
    TEST_TASK_ASSERT((int)(real_time - clock_time) - diff >= 0);
    TEST_TASK_ASSERT(clock_time == clock_start_time + 20);

    yield();
}

int _test_clock()
{
    task_t kernel_task;
    kernel_task.tid = 0;

    task_t tasks[CLOCK_TEST_NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, CLOCK_TEST_NUM_TASKS);
    char* stack = USER_STACK_START;

    uint64_t n_tasks = 1;
    task_t* task1 = allocator_new_task(&allocator, stack, n_tasks++, 1, &clock_task1func, &kernel_task);
    task_t* name_server = allocator_new_task(&allocator, stack, n_tasks++, 1, &name_server_task, &kernel_task);
    task_t* clock_server = allocator_new_task(&allocator, stack, n_tasks++, 1, &k3_clock_server, &kernel_task);
    task_t* clock_notifier = allocator_new_task(&allocator, stack, n_tasks++, 1, &k3_clock_notifier, &kernel_task);
    task_t* idle_task = allocator_new_task(&allocator, stack, n_tasks++, 1, &idletaskfunc, &kernel_task);

    priority_queue_t scheduler = pq_new();
    queue_t tasks_waiting_for_send = queue_new();
    queue_t tasks_waiting_for_reply = queue_new();
    queue_t tasks_waiting_for_timer = queue_new();
    queue_t tasks_waiting_for_terminal = queue_new();

    uint32_t next_tick = timer_get_us() + US_PER_TICK;

    main_context_t context;
    context.kernel_task = &kernel_task;
    context.allocator = &allocator;
    context.stack = stack;
    context.n_tasks = &n_tasks;
    context.scheduler = &scheduler;
    context.tasks_waiting_for_send = &tasks_waiting_for_send;
    context.tasks_waiting_for_reply = &tasks_waiting_for_reply;
    context.tasks_waiting_for_timer = &tasks_waiting_for_timer;
    context.tasks_waiting_for_terminal = &tasks_waiting_for_terminal;
    context.next_tick = next_tick;

    uint64_t syndrome;

    // Set up name server, clock server, clock notifier.
    SEND(clock_server);
    RECEIVE_REPLY(name_server);

    SEND(clock_notifier);
    RECEIVE_REPLY(name_server);

    // Test time() with invalid tid
    SEND(task1);
    RECEIVE_REPLY(name_server);

    // Test time()
    SEND(task1);
    RECEIVE_REPLY(name_server);

    SEND(task1);
    RECEIVE_REPLY(clock_server);

    // Test delay() with negative time
    SEND(task1);
    RECEIVE_REPLY(name_server);

    // Test delay()
    SEND(task1);
    RECEIVE_REPLY(name_server);

    SEND(task1);
    context.active_task = clock_server;
    syndrome = enter_task(&kernel_task, clock_server) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_RECEIVE);
    receive_handler(&context);

    for (int i = 0; i < 10; ++i) {
        context.active_task = clock_notifier;
        syndrome = enter_task(&kernel_task, clock_notifier) & 0xFFFF;
        TEST_ASSERT(syndrome == SYSCALL_AWAIT_EVENT);
        await_event_handler(&context);

        context.active_task = idle_task;
        syndrome = enter_task(&kernel_task, idle_task) & 0xFFFF;
        TEST_ASSERT(syndrome == INTERRUPT_CODE);
        handle_interrupt(&context);

        SEND(clock_notifier);
        RECEIVE_REPLY(clock_server);
    }

    context.active_task = clock_server;
    syndrome = enter_task(&kernel_task, clock_server) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_REPLY);
    reply_handler(&context);

    SEND(task1);
    RECEIVE_REPLY(name_server);

    SEND(task1);
    RECEIVE_REPLY(clock_server);

    // Test delay_until() with past time;
    SEND(task1);
    RECEIVE_REPLY(name_server);

    SEND(task1);
    RECEIVE_REPLY(clock_server);

    // Test delay_until()
    SEND(task1);
    RECEIVE_REPLY(name_server);

    SEND(task1);
    context.active_task = clock_server;
    syndrome = enter_task(&kernel_task, clock_server) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_RECEIVE);
    receive_handler(&context);

    for (int i = 0; i < 10; ++i) {
        context.active_task = clock_notifier;
        syndrome = enter_task(&kernel_task, clock_notifier) & 0xFFFF;
        TEST_ASSERT(syndrome == SYSCALL_AWAIT_EVENT);
        await_event_handler(&context);

        context.active_task = idle_task;
        syndrome = enter_task(&kernel_task, idle_task) & 0xFFFF;
        TEST_ASSERT(syndrome == INTERRUPT_CODE);
        handle_interrupt(&context);

        SEND(clock_notifier);
        RECEIVE_REPLY(clock_server);
    }

    context.active_task = clock_server;
    syndrome = enter_task(&kernel_task, clock_server) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_REPLY);
    reply_handler(&context);

    SEND(task1);
    RECEIVE_REPLY(name_server);

    SEND(task1);
    RECEIVE_REPLY(clock_server);

    syndrome = enter_task(&kernel_task, task1) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);
    return 1;
}

int run_clock_tests()
{
    TEST_RUN(_test_clock);
    return 1;
}

#endif /* clock_test.h */
