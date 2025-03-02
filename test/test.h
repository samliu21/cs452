#ifndef _test_h_
#define _test_h_

#include "allocator_test.h"
#include "clock_test.h"
#include "context_switch_test.h"
#include "message_test.h"
#include "priority_queue_pi_test.h"
#include "priority_queue_task_test.h"
#include "queue_task_test.h"
#include "stringmap_test.h"
#include "syscall_test.h"
#include "uintmap_test.h"

#define execute(test, failed) \
    {                         \
        if (test() == 0) {    \
            failed = 1;       \
        }                     \
    }

int tests()
{
    int failed = 0;
    uart_puts(CONSOLE, "\r\n\r\n-------------------------\r\nRunning tests...\r\n");
    execute(run_allocator_tests, failed);
    execute(run_pq_task_tests, failed);
    execute(run_pq_pi_tests, failed);
    execute(run_queue_task_tests, failed);
    execute(run_stringmap_tests, failed);
    execute(run_uintmap_tests, failed);
    execute(run_context_switch_tests, failed);
    execute(run_syscall_tests, failed);
    execute(run_message_tests, failed);
    execute(run_clock_tests, failed);
    uart_puts(CONSOLE, "Tests complete.\r\n-------------------------\r\n\r\n");
    return failed;
}

#endif
