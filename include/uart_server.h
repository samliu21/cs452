#ifndef _uart_server_h_
#define _uart_server_h_

#include "charqueue.h"
#include "interrupt.h"
#include "name_server.h"
#include "syscall_func.h"

typedef enum {
    REQUEST_UART_READ = 'R',
    REQUEST_UART_WRITE = 'W',
    REQUEST_READ_INTERRUPT = 'E',
    REQUEST_WRITE_INTERRUPT = 'F',
} uart_request_t;

int getc(uint64_t tid, int channel)
{
    uint64_t uart_server_tid = who_is("uart_server");
    if (tid != uart_server_tid) {
        return -1;
    }
    char c;
    char msg = REQUEST_UART_READ;
    int64_t ret;
    if (channel == CONSOLE) {
        ret = send(uart_server_tid, &msg, 1, &c, 1);
    } else {
        ret = -1;
    }
    ASSERT(ret >= 0, "send failed");
    return c;
}

void k4_uart_server()
{
    int64_t res = register_as("uart_server");
    ASSERT(res >= 0, "register_as failed");

    char argv[4][32];
    char msg[32];

    charqueuenode readnodes[256], writenodes[256];
    charqueue readqueue = charqueue_new(readnodes, 256);
    charqueue writequeue = charqueue_new(writenodes, 256);
    uint64_t reader_tid = 0; // assume there is only one task that reads from console

    for (;;) {
        uint64_t caller_tid;
        int64_t msglen = receive(&caller_tid, msg, 32);
        msg[msglen] = 0;
        int argc = split(msg, argv);

        switch (argv[0][0]) {
        case REQUEST_UART_READ: {
            ASSERT(argc == 1, "uart_read expects no parameters");

            // if we have a char in the buffer, return it immediately
            if (!charqueue_empty(&readqueue)) {
                char c = charqueue_pop(&readqueue);
                reply(caller_tid, &c, 1);
                break;
            }
            enable_uart_read_interrupts();
            reader_tid = caller_tid;
            break;
        }
        case REQUEST_UART_WRITE: {
            break;
        }
        case REQUEST_READ_INTERRUPT: {
            charqueue_add(&readqueue, uart_assert_getc(CONSOLE));
            ASSERT(reader_tid != 0, "reader_tid doesn't exist");

            // respond to reader
            char c = charqueue_pop(&readqueue);
            int64_t res = reply(reader_tid, &c, 1);
            ASSERTF(res >= 0, "reply failed with code: %d", res);
            reader_tid = 0;

            // respond to notifier
            reply_empty(caller_tid);
            break;
        }
        case REQUEST_WRITE_INTERRUPT: {
            // TODO: look into repeatedly writing to FIFO output buffer
            if (!charqueue_empty(&writequeue)) {
                uart_assert_putc(CONSOLE, charqueue_pop(&writequeue));
            }
            break;
        }
        }
    }
}

#endif
