#ifndef _test_h_
#define _test_h_

#include "slab_test.h"

void tests()
{
    uart_puts(CONSOLE, "\r\n\r\n-------------------------\r\nRunning tests...\r\n");
    slab_tests();
    uart_puts(CONSOLE, "Tests complete.\r\n-------------------------\r\n\r\n");
}

#endif
