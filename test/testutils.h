#ifndef _test_utils_h_
#define _test_utils_h_

#include "rpi.h"

#define TEST_ASSERT(x)                                   \
    if (!(x)) {                                          \
        uart_puts(CONSOLE, "assert failed: " #x "\r\n"); \
        return 0;                                        \
    }
#define TEST_RUN(x)                                    \
    if (!x()) {                                        \
        uart_puts(CONSOLE, "test failed: " #x "\r\n"); \
    }

#endif