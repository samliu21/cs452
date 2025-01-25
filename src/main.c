#include "allocator.h"
#include "common.h"
#include "exception.h"
#include "priority_queue.h"
#include "rpi.h"
#include "syscall_asm.h"
#include "task.h"
#include "test.h"
#include "user_tasks.h"

void send_receive(task_t* sender, task_t* receiver)
{
    uint64_t* caller_tid = (uint64_t*)receiver->registers[0];
    *caller_tid = sender->tid;

    // copy over sender's message to receiver's buffer and return to receiver length of message
    uint64_t n = (receiver->registers[2] < sender->registers[2]) ? receiver->registers[2] : sender->registers[2];
    memcpy((void*)receiver->registers[1], (void*)sender->registers[1], n);
    receiver->registers[0] = n;
}

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
    task_t* initial_task = allocator_new_task(&allocator, stack, n_tasks++, 1, &k2_initial_user_task, &kernel_task);

    // create PQ with initial task in it
    priority_queue_t scheduler = pq_new();
    pq_add(&scheduler, initial_task);

    queue_t tasks_waiting_for_send = queue_new();
    queue_t tasks_waiting_for_reply = queue_new();

    task_t* active_task;
    while (!pq_empty(&scheduler)) {
        active_task = pq_pop(&scheduler);
        ASSERT(active_task->state == READY, "active task is not in ready state");
        // uart_printf(CONSOLE, "active task: %u\r\n", active_task->tid);

        uint64_t esr = enter_task(&kernel_task, active_task);

        uint64_t syndrome = esr & 0xFFFF;

        // uart_printf(CONSOLE, "syscall with code: %u\r\n", syndrome);

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
            task_t* receiver = get_task(tasks_waiting_for_send.head, active_task->registers[0]);
            // uart_printf(CONSOLE, "receiver size: %u\r\n", tasks_waiting_for_send.size);
            // if (receiver != NULL) {
            //     uart_puts(CONSOLE, "huh?\r\n");
            // }

            if (receiver) { // the receiver is already waiting for a message, so send
                ASSERT(receiver->state == RECEIVEWAIT, "blocked receiver is not in receive wait state");

                send_receive(active_task, receiver);

                // unblock receiver
                queue_delete(&tasks_waiting_for_send, receiver);
                pq_add(&scheduler, receiver);
                receiver->state = READY;

                // block sender
                queue_add(&tasks_waiting_for_reply, active_task);
                active_task->state = REPLYWAIT;
            } else { // the receiver has NOT requested a message
                receiver = get_task(scheduler.head, active_task->registers[0]);
                ASSERT(receiver != NULL, "receiver not allocated");
                queue_add(&(receiver->senders_queue), active_task);
                active_task->state = SENDWAIT;
            }
            break;
        }
        case SYSCALL_RECEIVE: {
            if (queue_empty(&active_task->senders_queue)) { // no senders, wait
                active_task->state = RECEIVEWAIT;
                queue_add(&tasks_waiting_for_send, active_task);
            } else { // there is a sender, fulfill their request
                task_t* sender = queue_pop(&active_task->senders_queue);
                ASSERT(sender->state == SENDWAIT, "blocked sender is not in send wait state");

                send_receive(sender, active_task);

                // unblock sender
                queue_add(&tasks_waiting_for_reply, sender);
                sender->state = REPLYWAIT;
            }
            break;
        }
        case SYSCALL_REPLY: {
            task_t* sender = get_task(tasks_waiting_for_reply.head, active_task->registers[0]);
            ASSERTF(sender != NULL, "sender %d could not be found in queue", active_task->registers[0]);
            ASSERT(sender->state == REPLYWAIT, "blocked sender is not in reply wait state");

            // copy over reply message to sender's buffer and return to both sender and replyer the length.
            uint64_t n = (active_task->registers[2] < sender->registers[4]) ? active_task->registers[2] : sender->registers[4];
            memcpy((void*)sender->registers[3], (void*)active_task->registers[1], n);
            sender->registers[0] = n;
            active_task->registers[0] = n;

            // unblock sender
            queue_delete(&tasks_waiting_for_reply, sender);
            pq_add(&scheduler, sender);
            sender->state = READY;
            break;
        }
        default: {
            ASSERT(0, "unrecognized syscall\r\n");
            for (;;) { }
        }
        }

        if (syndrome != SYSCALL_EXIT && active_task->state == READY) {
            pq_add(&scheduler, active_task);
        }
    }

    uart_puts(CONSOLE, "No tasks to run\r\n");
    for (;;) { }
}
