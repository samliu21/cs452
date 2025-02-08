#ifndef _uart_server_h_
#define _uart_server_h_

#include "common.h"

typedef enum {
    REQUEST_UART_READ = 'R',
    REQUEST_UART_WRITE = 'W',
    REQUEST_READ_AVAILABLE = 'r',
    REQUEST_WRITE_AVAILABLE = 'w',
    REQUEST_CTS_AVAILABLE = 'c',
} uart_request_t;

int64_t getc(uint64_t tid, int channel);
int64_t putc(uint64_t tid, int channel, char c);
int64_t puts(uint64_t tid, int channel, const char* buf);
int64_t printf(uint64_t line, int channel, const char* fmt, ...);
void k4_uart_server();

#endif
