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

int64_t getc(uint64_t tid, int channel);
int64_t putc(uint64_t tid, int channel, char c);
int64_t puts(uint64_t tid, int channel, const char* buf);
int64_t printf(uint64_t line, int channel, const char* fmt, ...);
void uart_server_task();

#endif
