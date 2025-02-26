#ifndef _uart_server_h_
#define _uart_server_h_

#include "common.h"

typedef enum {
    REQUEST_UART_READ = 1,
    REQUEST_UART_WRITE = 2,
    REQUEST_UART_WRITE_STRING = 3,
    REQUEST_READ_AVAILABLE = 4,
    REQUEST_WRITE_AVAILABLE = 5,
    REQUEST_CTS_AVAILABLE = 6,
} uart_request_t;

int64_t getc(int channel);
int64_t putc(int channel, char c);
int64_t puts(int channel, const char* buf);
int64_t printf(int channel, const char* fmt, ...);
void uart_server_task();

#endif
