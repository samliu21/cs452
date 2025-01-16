#ifndef _test_h_
#define _test_h_

#include "priority_queue_test.h"
#include "slab_test.h"

void tests()
{
    uart_puts(CONSOLE, "\r\n\r\n-------------------------\r\nRunning tests...\r\n");
    slab_tests();
    pq_tests();
    uart_puts(CONSOLE, "Tests complete.\r\n-------------------------\r\n\r\n");
}

#endif
