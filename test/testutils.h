#ifndef _test_utils_h_
#define _test_utils_h_

#include "rpi.h"

#define TEST_STACK_SIZE 1024

#define TEST_ASSERT(x)                                   \
    if (!(x)) {                                          \
        uart_puts(CONSOLE, "assert failed: " #x "\r\n"); \
        return 0;                                        \
    }
#define TEST_TASK_ASSERT(x)                              \
    if (!(x)) {                                          \
        uart_puts(CONSOLE, "assert failed: " #x "\r\n"); \
        exit();                                          \
    }
#define TEST_RUN(x)                                    \
    if (!x()) {                                        \
        uart_puts(CONSOLE, "test failed: " #x "\r\n"); \
        return 0;                                      \
    }
#define SEND(x)                                      \
    context.active_task = x;                         \
    syndrome = enter_task(&kernel_task, x) & 0xFFFF; \
    TEST_ASSERT(syndrome == SYSCALL_SEND);           \
    send_handler(&context);                          
#define RECEIVE_REPLY(x)                             \
    context.active_task = x;                         \
    syndrome = enter_task(&kernel_task, x) & 0xFFFF; \
    TEST_ASSERT(syndrome == SYSCALL_RECEIVE);        \
    receive_handler(&context);                       \
    syndrome = enter_task(&kernel_task, x) & 0xFFFF; \
    TEST_ASSERT(syndrome == SYSCALL_REPLY);          \
    reply_handler(&context);                         

#endif