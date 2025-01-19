#ifndef _user_tasks_
#define _user_tasks_

void test()
{
    uart_puts(CONSOLE, "hello from test!\r\n");
    uint64_t tid = my_tid();
    uart_printf(CONSOLE, "test has tid: %u\r\n", tid);
    uint64_t parent_tid = my_parent_tid();
    uart_printf(CONSOLE, "test has parent tid: %u\r\n", parent_tid);
    for (;;) { }
}

void foo()
{
    uart_puts(CONSOLE, "hello from foo!\r\n");
    uint64_t tid = my_tid();
    uart_printf(CONSOLE, "foo has tid: %u\r\n", tid);

    create((uint64_t)priority_1, (uint64_t)&test);

    for (;;) { }
}

#endif
