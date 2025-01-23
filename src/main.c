#include "allocator.h"
#include "exception.h"
#include "priority_queue.h"
#include "rpi.h"
#include "stack.h"
#include "syscall.h"
#include "task.h"
#include "test.h"
#include "user_tasks.h"

int kmain()
{
    gpio_init();
    uart_config_and_enable(CONSOLE);
    uart_puts(CONSOLE, "\r\nconsole loaded!\r\n");

    // run tests and initialize exception vector
    init_vbar();
    tests();

    // create kernel task
    task_t kernel_task;
    kernel_task.tid = 0;

    // create allocator
    task_t tasks[NUM_TASKS];
    allocator_t allocator = allocator_new(tasks, NUM_TASKS);
    char stack[NUM_TASKS * STACK_SIZE];

    // create initial task
    uint64_t n_tasks = 1;
    task_t* initial_task = allocator_new_task(&allocator, stack, n_tasks++, 1, &k1_initial_user_task, &kernel_task);

    // create PQ with initial task in it
    priority_queue_t scheduler = pq_new();
    pq_add(&scheduler, initial_task);

    queue_t blocked = queue_new();

    task_t* active_task;
    while (!pq_empty(&scheduler)) {
        active_task = pq_pop(&scheduler);

        uint64_t esr = enter_task(&kernel_task, active_task);

        uint64_t syndrome = esr & 0xFFFF;

        switch (syndrome) {
        case SYSCALL_CREATE: {
            uint64_t priority = (uint64_t)active_task->registers[0];
            func_t entry_point = (func_t)active_task->registers[1];
            if (allocator.n_free > 0) {
                task_t* new_task = allocator_new_task(&allocator, stack, n_tasks++, priority, entry_point, active_task);
                pq_add(&scheduler, new_task);
                active_task->registers[0] = new_task->tid;
            } else {
                active_task->registers[0] = -2;
            }
            break;
        }
        case SYSCALL_MYTID: {
            active_task->registers[0] = active_task->tid;
            break;
        }
        case SYSCALL_MYPARENTTID: {
            active_task->registers[0] = active_task->parent_tid;
            break;
        }
        case SYSCALL_YIELD: {
            break;
        }
        case SYSCALL_EXIT: {
            allocator_free(&allocator, active_task);
            break;
        }
        case SYSCALL_SEND: {
            task_t* receiver = get_task(blocked.head, active_task->registers[0]);
            if (receiver && receiver->state == RECEIVEWAIT) {
                uint64_t n = (receiver->registers[2] < active_task->registers[2]) ? receiver->registers[2] : active_task->registers[2];
                memcpy((void*)receiver->registers[1], (void*)active_task->registers[1], n);
                receiver->registers[0] = n;
                queue_delete(&blocked, receiver);
                pq_add(&scheduler, receiver);
            } else {
                if (!receiver) {
                    receiver = get_task(active_task->next_task, active_task->registers[0]);
                }
                queue_add(&(receiver->senders_queue), active_task);
            }
            break;
        }
        case SYSCALL_RECEIVE: {
            break;
        }
        case SYSCALL_REPLY: {
            break;
        }
        default: {
            ASSERT(0, "unrecognized syscall\r\n");
            for (;;) { }
        }
        }

        if (syndrome != SYSCALL_EXIT) {
            pq_add(&scheduler, active_task);
        }
    }

    uart_puts(CONSOLE, "No tasks to run\r\n");
    for (;;) { }
}
