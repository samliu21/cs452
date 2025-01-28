#ifndef _send_receive_test_h_
#define _send_receive_test_h_

#include "allocator.h"
#include "common.h"
#include "syscall_asm.h"
#include "syscall_handler.h"
#include "testutils.h"

#define MESSAGE_TEST_NUM_TASKS 2

void senderfunc()
{
    char reply_buf[6];
    int64_t n = send(2, "hello", 6, reply_buf, 5);
    TEST_TASK_ASSERT(n == 4);
    TEST_TASK_ASSERT(strcmp(reply_buf, "hell"));

    yield();

    n = send(2, "yo", 2, reply_buf, 2);
    TEST_TASK_ASSERT(n == 2);
    TEST_TASK_ASSERT(strcmp(reply_buf, "yo"));

    n = send(42, "yo", 2, reply_buf, 2);
    TEST_TASK_ASSERT(n == -1);

    yield();
}

void receiverfunc()
{
    uint64_t tid;
    char receive_buf[5];
    int64_t n = receive(&tid, receive_buf, 5);
    TEST_TASK_ASSERT(tid == 1);
    TEST_TASK_ASSERT(n == 4);
    TEST_TASK_ASSERT(strcmp(receive_buf, "hell"));
    n = reply(tid, receive_buf, 4);
    TEST_TASK_ASSERT(n == 4)

    n = receive(&tid, receive_buf, 4);
    TEST_TASK_ASSERT(tid == 1);
    TEST_TASK_ASSERT(n == 2);
    TEST_TASK_ASSERT(strcmp(receive_buf, "yo"));
    n = reply(tid, "what's up?", 9);
    TEST_TASK_ASSERT(n == 2)

    n = reply(42, receive_buf, 2);
    TEST_TASK_ASSERT(n == -1);

    n = reply(1, receive_buf, 2);
    TEST_TASK_ASSERT(n == -2);

    yield();
}

int _test_message()
{
    task_t kernel_task;
    kernel_task.tid = 0;

    task_t tasks[MESSAGE_TEST_NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, MESSAGE_TEST_NUM_TASKS);
    char stack[MESSAGE_TEST_NUM_TASKS * TEST_STACK_SIZE];
    uint64_t n_tasks = 1;
    task_t* sender = allocator_new_task(&allocator, stack, n_tasks++, 1, &senderfunc, &kernel_task);
    task_t* receiver = allocator_new_task(&allocator, stack, n_tasks++, 1, &receiverfunc, &kernel_task);

    priority_queue_t pq;
    queue_t send_queue;
    queue_t reply_queue;

    main_context_t context;
    context.kernel_task = &kernel_task;
    context.allocator = &allocator;
    context.stack = stack;
    context.n_tasks = &n_tasks;
    context.active_task = sender;
    context.scheduler = &pq;
    context.tasks_waiting_for_send = &send_queue;
    context.tasks_waiting_for_reply = &reply_queue;

    uint64_t syndrome;

    TEST_ASSERT(sender->state == READY);
    TEST_ASSERT(receiver->state == READY);

    syndrome = enter_task(&kernel_task, sender) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_SEND);
    send_handler(&context);
    TEST_ASSERT(sender->state == SENDWAIT);
    TEST_ASSERT(receiver->state == READY);

    context.active_task = receiver;
    syndrome = enter_task(&kernel_task, receiver) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_RECEIVE);
    receive_handler(&context);
    TEST_ASSERT(context.tasks_waiting_for_reply->head = sender);
    TEST_ASSERT(sender->state == REPLYWAIT);
    TEST_ASSERT(receiver->state == READY);

    syndrome = enter_task(&kernel_task, receiver) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_REPLY);
    reply_handler(&context);
    TEST_ASSERT(sender->state == READY);
    TEST_ASSERT(receiver->state == READY);

    context.active_task = sender;
    syndrome = enter_task(&kernel_task, sender) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);

    context.active_task = receiver;
    syndrome = enter_task(&kernel_task, receiver) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_RECEIVE);
    receive_handler(&context);
    TEST_ASSERT(sender->state == READY);
    TEST_ASSERT(context.tasks_waiting_for_send->head = receiver);
    TEST_ASSERT(receiver->state == RECEIVEWAIT);

    context.active_task = sender;
    syndrome = enter_task(&kernel_task, sender) & 0xFFFF;
    uart_printf(CONSOLE, "2\r\n");
    TEST_ASSERT(syndrome == SYSCALL_SEND);
    send_handler(&context);
    TEST_ASSERT(context.tasks_waiting_for_reply->head = sender);
    TEST_ASSERT(sender->state == REPLYWAIT);
    TEST_ASSERT(receiver->state == READY);

    context.active_task = receiver;
    syndrome = enter_task(&kernel_task, receiver) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_REPLY);
    reply_handler(&context);
    TEST_ASSERT(sender->state == READY);
    TEST_ASSERT(receiver->state == READY);

    context.active_task = sender;
    syndrome = enter_task(&kernel_task, sender) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);

    context.active_task = receiver;

    syndrome = enter_task(&kernel_task, receiver) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_REPLY);

    syndrome = enter_task(&kernel_task, receiver) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_REPLY);

    syndrome = enter_task(&kernel_task, receiver) & 0xFFFF;
    TEST_ASSERT(syndrome == SYSCALL_YIELD);

    return 1;
}

void run_message_tests()
{
    TEST_RUN(_test_message);
}

#endif
