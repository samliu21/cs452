#include "uart_server.h"
#include "charqueue.h"
#include "clock_server.h"
#include "interrupt.h"
#include "name_server.h"
#include "syscall_func.h"
#include <stdarg.h>

#define WRITER_QUEUE_DUMMY_TID 0

int64_t getc(int channel)
{
    if (channel != CONSOLE && channel != MARKLIN) {
        return -1;
    }
    int64_t tid = (channel == CONSOLE) ? who_is(TERMINAL_TASK_NAME) : who_is(MARKLIN_TASK_NAME);
    ASSERT(tid >= 0, "who_is failed");

    char c;
    char msg = REQUEST_UART_READ;
    int64_t ret = send(tid, &msg, 1, &c, 1);
    ASSERT(ret >= 0, "getc send failed");
    return c;
}

int64_t putc(int channel, char c)
{
    if (channel != CONSOLE && channel != MARKLIN) {
        return -1;
    }
    int64_t tid = (channel == CONSOLE) ? who_is(TERMINAL_TASK_NAME) : who_is(MARKLIN_TASK_NAME);
    ASSERT(tid >= 0, "who_is failed");

    char buf[2];
    buf[0] = REQUEST_UART_WRITE;
    buf[1] = c;
    int64_t ret = send(tid, buf, 2, NULL, 0);
    ASSERT(ret >= 0, "putc send failed");
    return 0;
}

int64_t puts(int channel, const char* buf)
{
    if (channel != CONSOLE && channel != MARKLIN) {
        return -1;
    }
    int64_t tid = (channel == CONSOLE) ? who_is(TERMINAL_TASK_NAME) : who_is(MARKLIN_TASK_NAME);
    ASSERT(tid >= 0, "who_is failed");

    int len = strlen(buf);
    char sendbuf[len + 2];
    sendbuf[0] = REQUEST_UART_WRITE_STRING;
    strcpy(sendbuf + 1, buf);
    sendbuf[len + 1] = 0;
    int64_t ret = send(tid, sendbuf, len + 2, NULL, 0);
    ASSERT(ret >= 0, "puts send failed");
    return 0;
}

int64_t va_printf(int channel, const char* fmt, va_list va)
{
    char ch, buf[12];
    int width = 0, pad_zero = 0;
    int64_t res = 0;
    char output[256];
    memset(output, 0, 256);
    int pos = 0;
    char* cur;

    while ((ch = *(fmt++))) {
        if (ch != '%') {
            output[pos++] = ch;
        } else {
            // Reset width and padding flag
            width = 0;
            pad_zero = 0;

            // Parse width specifier (e.g., %2u, %03d)
            ch = *(fmt++);
            if (ch == '0') { // Zero-padding detected
                pad_zero = 1;
                ch = *(fmt++);
            }
            while (ch >= '0' && ch <= '9') { // Extract width
                width = width * 10 + (ch - '0');
                ch = *(fmt++);
            }

            switch (ch) {
            case 'u':
                ui2a(va_arg(va, unsigned int), 10, buf);
                break;
            case 'd':
                i2a(va_arg(va, int), buf);
                break;
            case 'x':
                ui2a(va_arg(va, unsigned int), 16, buf);
                break;
            case 's':
                cur = va_arg(va, char*);
                while (*cur) {
                    output[pos++] = *cur;
                    cur++;
                }
                continue;
            case '%':
                output[pos++] = '%';
                continue;
            case '\0':
                return 0; // End of format string
            default:
                output[pos++] = ch;
                continue;
            }

            // Get length of formatted number
            int len = strlen(buf);

            // Handle padding (either '0' or ' ')
            while (len < width) {
                output[pos++] = pad_zero ? '0' : ' ';
                len++;
            }

            // Print formatted number
            cur = buf;
            while (*cur) {
                output[pos++] = *cur;
                cur++;
            }
        }
    }
    output[pos] = 0;
    puts(channel, output);
    return res;
}

int64_t printf(int channel, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    va_printf(channel, fmt, va);
    va_end(va);
    return 0;
}

int64_t log(const char* fmt, ...)
{
    printf(CONSOLE, "\033[s\033[17;60r\033[60;1H[%d] ", time());

    va_list va;
    va_start(va, fmt);
    va_printf(CONSOLE, fmt, va);
    va_end(va);

    printf(CONSOLE, "\033[61;999r\033[u");

    return 0;
}

int64_t warn(const char* fmt, ...)
{
    printf(CONSOLE, "\033[s\033[17;60r\033[60;1H\033[33m[%d] ", time());

    va_list va;
    va_start(va, fmt);
    va_printf(CONSOLE, fmt, va);
    va_end(va);

    printf(CONSOLE, "\033[37m\033[61;999r\033[u");

    return 0;
}

void uart_server_task()
{
    uint64_t initial_task_tid;
    char type;
    int64_t res = receive(&initial_task_tid, &type, 1);
    ASSERT(res >= 0, "receive failed");
    res = reply_empty(initial_task_tid);
    ASSERT(res >= 0, "reply failed");
    int line = type;
    ASSERT(line == CONSOLE || line == MARKLIN, "invalid line");

    res = register_as(line == CONSOLE ? TERMINAL_TASK_NAME : MARKLIN_TASK_NAME);
    ASSERT(res >= 0, "register_as failed");

    char msg[512];
    charqueuenode writenodes[2048], readertidnodes[1024];
    charqueue writequeue = charqueue_new(writenodes, 2048);
    charqueue readertidqueue = charqueue_new(readertidnodes, 1024);
    uint64_t caller_tid;

    int cts_flag = 1;

    for (;;) {
        int64_t ret = receive(&caller_tid, msg, 512);
        ASSERT(ret >= 0, "receive failed");

        switch (msg[0]) {
        case REQUEST_UART_READ: {
            if (uart_read_available(line)) {
                // if we have a char in the uart, return it immediately
                char c = uart_assert_getc(line);
                reply(caller_tid, &c, 1);
            } else {
                // otherwise, enable read interrupts and wait for the interrupt
                enable_uart_read_interrupts(line);
                charqueue_add(&readertidqueue, caller_tid);
            }
            break;
        }
        case REQUEST_UART_WRITE: {
            reply_empty(caller_tid);
            char c = msg[1];
            charqueue_add(&writequeue, c);

            break;
        }
        case REQUEST_UART_WRITE_STRING: {
            reply_empty(caller_tid);
            char* s = msg + 1;
            while (*s) {
                charqueue_add(&writequeue, *s);
                s++;
            }

            break;
        }
        case REQUEST_READ_AVAILABLE: {
            ASSERT(!charqueue_empty(&readertidqueue), "reader tid queue is empty");

            // respond to reader
            char c = uart_assert_getc(line);
            int64_t res = reply(charqueue_pop(&readertidqueue), &c, 1);
            ASSERTF(res >= 0, "reply failed with code: %d", res);

            // respond to notifier
            res = reply_empty(caller_tid);
            ASSERT(res >= 0, "reply failed");
            break;
        }
        case REQUEST_WRITE_AVAILABLE: {
            int64_t res = reply_empty(caller_tid);
            ASSERT(res >= 0, "reply failed");
            break;
        }
        case REQUEST_CTS_AVAILABLE: {
            ASSERT(line == MARKLIN, "CTS is only available for marklin");

            uint32_t fr = UART_REG(line, UART_FR);
            if (fr & UART_FR_CTS) {
                cts_flag = 1;
            }
            int64_t res = reply_empty(caller_tid);
            ASSERT(res >= 0, "reply failed");
            break;
        }
        }

        for (;;) {
            int tx_flag = uart_write_available(line);
            int channel_ready = tx_flag;
            if (line == MARKLIN) {
                channel_ready = channel_ready && cts_flag;
            }

            if (charqueue_empty(&writequeue)) {
                break;
            } else if (!channel_ready) {
                enable_uart_write_interrupts(line);
                break;
            }

            uart_assert_putc(line, charqueue_pop(&writequeue));

            cts_flag = 0;
        }
    }
}
