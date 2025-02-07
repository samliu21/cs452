#include "uart_server.h"
#include "charqueue.h"
#include "interrupt.h"
#include "name_server.h"
#include "syscall_func.h"

int64_t getc(uint64_t tid, int channel)
{
    uint64_t terminal_tid = who_is("uart_terminal_server");
    uint64_t marklin_tid = who_is("uart_marklin_server");
    if (tid != terminal_tid && tid != marklin_tid) {
        return -1;
    }
    if ((tid == terminal_tid && channel != CONSOLE) || (tid == marklin_tid && channel != MARKLIN)) {
        return -1;
    }
    char c;
    char msg = REQUEST_UART_READ;
    int64_t ret = send(tid, &msg, 1, &c, 1);
    ASSERT(ret >= 0, "send failed");
    return c;
}

int64_t putc(uint64_t tid, int channel, char c)
{
    uint64_t terminal_tid = who_is("uart_terminal_server");
    uint64_t marklin_tid = who_is("uart_marklin_server");
    if (tid != terminal_tid && tid != marklin_tid) {
        return -1;
    }
    if ((tid == terminal_tid && channel != CONSOLE) || (tid == marklin_tid && channel != MARKLIN)) {
        return -1;
    }
    char buf[2];
    buf[0] = REQUEST_UART_WRITE;
    buf[1] = c;
    int64_t ret = send(tid, buf, 2, NULL, 0);
    ASSERT(ret >= 0, "send failed");
    return 0;
}

int64_t puts(uint64_t tid, int channel, const char* buf)
{
    int64_t res = 0;
    while (buf[res] != '\0') {
        if (putc(tid, channel, buf[res]) == -1) {
            res = -1;
            break;
        }
        res++;
    }
    return res;
}

void k4_uart_marklin_server()
{
    int64_t res = register_as("uart_marklin_server");
    ASSERT(res >= 0, "register_as failed");

    char msg[32];
    charqueuenode writenodes[256];
    charqueue writequeue = charqueue_new(writenodes, 256);
    uint64_t reader_tid = 0;
    uint64_t writer_tid = 0;
    uint64_t caller_tid;

    int cts_flag = 1;

    for (;;) {
        int64_t ret = receive(&caller_tid, msg, 32);
        ASSERT(ret >= 0, "receive failed");

        switch (msg[0]) {
        case REQUEST_UART_READ: {
            if (uart_read_available(MARKLIN)) {
                // if we have a char in the uart, return it immediately
                char c = uart_assert_getc(MARKLIN);
                reply(caller_tid, &c, 1);
            } else {
                // otherwise, enable read interrupts and wait for the interrupt
                enable_uart_read_interrupts(MARKLIN);
                reader_tid = caller_tid;
            }
            break;
        }
        case REQUEST_UART_WRITE: {
            char c = msg[1];
            charqueue_add(&writequeue, c);
            writer_tid = caller_tid;

            if (!uart_write_available(MARKLIN)) {
                enable_uart_write_interrupts(MARKLIN);
            }
            break;
        }
        case REQUEST_READ_AVAILABLE: {
            ASSERT(reader_tid != 0, "reader_tid doesn't exist");

            // respond to reader
            char c = uart_assert_getc(MARKLIN);
            int64_t res = reply(reader_tid, &c, 1);
            ASSERTF(res >= 0, "reply failed with code: %d", res);
            reader_tid = 0;

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
            uint32_t fr = UART_REG(MARKLIN, UART_FR);
            if (fr & UART_FR_CTS) {
                cts_flag = 1;
            }
            int64_t res = reply_empty(caller_tid);
            ASSERT(res >= 0, "reply failed");
            break;
        }
        }

        int tx_flag = uart_write_available(MARKLIN);
        if (tx_flag && cts_flag && !charqueue_empty(&writequeue)) {
            uart_assert_putc(MARKLIN, charqueue_pop(&writequeue));

            ASSERT(writer_tid != 0, "writer_tid doesn't exist");
            int64_t res = reply_empty(writer_tid);
            ASSERT(res >= 0, "reply failed");
            writer_tid = 0;

            cts_flag = 0;
        }
    }
}

void k4_uart_terminal_server()
{
    int64_t res = register_as("uart_terminal_server");
    ASSERT(res >= 0, "register_as failed");

    char msg[32];
    charqueuenode writenodes[256];
    charqueue writequeue = charqueue_new(writenodes, 256);
    uint64_t reader_tid = 0; // assume there is only one task that reads from console
    uint64_t writer_tid = 0;
    uint64_t caller_tid;

    for (;;) {
        int64_t ret = receive(&caller_tid, msg, 32);
        ASSERT(ret >= 0, "receive failed");

        switch (msg[0]) {
        case REQUEST_UART_READ: {
            if (uart_read_available(CONSOLE)) {
                // if we have a char in the uart, return it immediately
                char c = uart_assert_getc(CONSOLE);
                reply(caller_tid, &c, 1);
            } else {
                // otherwise, enable read interrupts and wait for the interrupt
                enable_uart_read_interrupts(CONSOLE);
                reader_tid = caller_tid;
            }
            break;
        }
        case REQUEST_UART_WRITE: {
            char c = msg[1];

            if (uart_write_available(CONSOLE)) {
                // if write channel is clear, write immediately
                uart_assert_putc(CONSOLE, c);
                reply_empty(caller_tid);
            } else {
                // otherwise, add to buffer and enable write interrupts
                enable_uart_write_interrupts(CONSOLE);
                writer_tid = caller_tid;
                charqueue_add(&writequeue, c);
            }

            break;
        }
        case REQUEST_READ_AVAILABLE: {
            ASSERT(reader_tid != 0, "reader_tid doesn't exist");

            // respond to reader
            char c = uart_assert_getc(CONSOLE);
            int64_t res = reply(reader_tid, &c, 1);
            ASSERTF(res >= 0, "reply failed with code: %d", res);
            reader_tid = 0;

            // respond to notifier
            res = reply_empty(caller_tid);
            ASSERT(res >= 0, "reply failed");
            break;
        }
        case REQUEST_WRITE_AVAILABLE: {
            ASSERT(writer_tid != 0, "writer_tid doesn't exist");

            // TODO: look into repeatedly writing to FIFO output buffer
            if (!charqueue_empty(&writequeue)) {
                uart_assert_putc(CONSOLE, charqueue_pop(&writequeue));
            }

            int64_t res = reply_empty(writer_tid);
            ASSERT(res >= 0, "reply failed");
            res = reply_empty(caller_tid);
            ASSERT(res >= 0, "reply failed");
            break;
        }
        }
    }
}
