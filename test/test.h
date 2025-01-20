#ifndef _test_h_
#define _test_h_

#include "allocator_test.h"
#include "priority_queue_test.h"

void tests()
{
	// TODO: add more tests where applicable
    uart_puts(CONSOLE, "\r\n\r\n-------------------------\r\nRunning tests...\r\n");
    run_allocator_tests();
    run_pq_tests();
    uart_puts(CONSOLE, "Tests complete.\r\n-------------------------\r\n\r\n");
}

#endif
