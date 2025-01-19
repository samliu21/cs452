#ifndef _user_tasks_
#define _user_tasks_

void spawned_task()
{
    uint64_t tid = my_tid();
    uint64_t parent_tid = my_parent_tid();
    uart_printf(CONSOLE, "\r\nspawned task has tid: %u, parent tid: %u\r\n", tid, parent_tid);

    yield();

    uart_printf(CONSOLE, "\r\nspawned task has tid: %u, parent tid: %u\r\n", tid, parent_tid);

    // exit();
    for (;;) { }
}

void initial_user_task()
{
    create((uint64_t)priority_0, (uint64_t)&spawned_task);
    create((uint64_t)priority_0, (uint64_t)&spawned_task);
    create((uint64_t)priority_2, (uint64_t)&spawned_task);
    create((uint64_t)priority_2, (uint64_t)&spawned_task);
}

// void test()
// {
//     uart_puts(CONSOLE, "\r\nhello from test!\r\n");
//     uint64_t tid = my_tid();
//     uart_printf(CONSOLE, "test has tid: %u\r\n", tid);

//     uint64_t parent_tid = my_parent_tid();
//     uart_printf(CONSOLE, "test has parent tid: %u\r\n", parent_tid);

//     for (;;) { }
// }

// void foo()
// {
//     uart_puts(CONSOLE, "\r\nhello from foo!\r\n");
//     uint64_t tid = my_tid();
//     uart_printf(CONSOLE, "foo has tid: %u\r\n", tid);

//     create((uint64_t)priority_1, (uint64_t)&test);

//     for (;;) { }
// }

#endif
