#include "syscall_handler.h"
#include "interrupt.h"
#include "rpi.h"
#include "timer.h"
#include "uintmap.h"
#include <stdlib.h>

#define IDLE_TIME_WINDOW_US (IDLE_TIME_WINDOW_TICKS * US_PER_TICK)

void send_receive(task_t* sender, task_t* receiver)
{
    uint64_t* caller_tid = (uint64_t*)receiver->registers[0];
    *caller_tid = sender->tid;

    // copy over sender's message to receiver's buffer and return to receiver length of message
    uint64_t n = (receiver->registers[2] < sender->registers[2]) ? receiver->registers[2] : sender->registers[2];
    memcpy((void*)receiver->registers[1], (void*)sender->registers[1], n);
    receiver->registers[0] = n;
}

void create_handler(main_context_t* context)
{
    uint64_t priority = (uint64_t)context->active_task->registers[0];
    func_t entry_point = (func_t)context->active_task->registers[1];
    if (context->allocator->n_free > 0) {
        task_t* new_task = allocator_new_task(context->allocator, context->stack, (*context->n_tasks)++, priority, entry_point, context->active_task);
        pq_task_add(context->scheduler, new_task);
        context->active_task->registers[0] = new_task->tid;
    } else {
        context->active_task->registers[0] = -2;
    }
}

void my_tid_handler(main_context_t* context)
{
    context->active_task->registers[0] = context->active_task->tid;
}

void my_parent_tid_handler(main_context_t* context)
{
    context->active_task->registers[0] = context->active_task->parent_tid;
}

void exit_handler(main_context_t* context)
{
    allocator_free(context->allocator, context->active_task);
}

void send_handler(main_context_t* context)
{
    task_t* receiver = get_task(context->tasks_waiting_for_send->head, context->active_task->registers[0]);

    if (receiver) { // the receiver is already waiting for a message, so send
        ASSERT(receiver->state == RECEIVEWAIT, "blocked receiver is not in receive wait state");

        send_receive(context->active_task, receiver);

        // unblock receiver
        queue_task_delete(context->tasks_waiting_for_send, receiver);
        pq_task_add(context->scheduler, receiver);
        receiver->state = READY;

        // block sender
        queue_task_add(context->tasks_waiting_for_reply, context->active_task);
        context->active_task->state = REPLYWAIT;
    } else { // the receiver has NOT requested a message
        receiver = allocator_get_task(context->allocator, context->active_task->registers[0]);
        if (receiver == NULL) {
            context->active_task->registers[0] = -1;
        } else {
            queue_task_add(&(receiver->senders_queue), context->active_task);
            context->active_task->state = SENDWAIT;
        }
    }
}

void receive_handler(main_context_t* context)
{
    if (queue_task_empty(&context->active_task->senders_queue)) { // no senders, wait
        context->active_task->state = RECEIVEWAIT;
        queue_task_add(context->tasks_waiting_for_send, context->active_task);
    } else { // there is a sender, fulfill their request
        task_t* sender = queue_task_pop(&context->active_task->senders_queue);
        ASSERT(sender->state == SENDWAIT, "blocked sender is not in send wait state");

        send_receive(sender, context->active_task);

        // unblock sender
        queue_task_add(context->tasks_waiting_for_reply, sender);
        sender->state = REPLYWAIT;
    }
}

void reply_handler(main_context_t* context)
{
    if (!allocator_get_task(context->allocator, context->active_task->registers[0])) {
        context->active_task->registers[0] = -1;
        return;
    }

    task_t* sender = get_task(context->tasks_waiting_for_reply->head, context->active_task->registers[0]);
    if (sender == NULL || sender->state != REPLYWAIT) {
        context->active_task->registers[0] = -2;
        return;
    }

    // copy over reply message to sender's buffer and return to both sender and replyer the length.
    uint64_t n = (context->active_task->registers[2] < sender->registers[4]) ? context->active_task->registers[2] : sender->registers[4];
    memcpy((void*)sender->registers[3], (void*)context->active_task->registers[1], n);
    sender->registers[0] = n;
    context->active_task->registers[0] = n;

    // unblock sender
    queue_task_delete(context->tasks_waiting_for_reply, sender);
    pq_task_add(context->scheduler, sender);
    sender->state = READY;
}

void await_event_handler(main_context_t* context)
{
    uint64_t event_type = context->active_task->registers[0];
    switch (event_type) {
    case EVENT_TICK: {
        timer_notify_at(context->next_tick);
        context->next_tick += US_PER_TICK;
        context->active_task->state = EVENTWAIT;
        queue_task_add(context->tasks_waiting_for_timer, context->active_task);
        break;
    }
    case EVENT_UART_TERMINAL: {
        context->active_task->state = EVENTWAIT;
        queue_task_add(context->tasks_waiting_for_terminal, context->active_task);
        break;
    }
    case EVENT_UART_MARKLIN: {
        context->active_task->state = EVENTWAIT;
        queue_task_add(context->tasks_waiting_for_marklin, context->active_task);
        break;
    }
    default:
        context->active_task->registers[0] = -1;
    }
}

void cpu_usage_handler(main_context_t* context)
{
    int t_end = timer_get_us();
    int t_start = t_end - IDLE_TIME_WINDOW_US;
    int window_length = min(IDLE_TIME_WINDOW_US, timer_get_us() - context->kernel_execution_start_time);

    while (!queue_pi_empty(context->kernel_time_queue) && context->kernel_time_queue->head->weight < t_start) {
        context->kernel_time -= queue_pi_pop(context->kernel_time_queue)->id;
    }
    while (!queue_pi_empty(context->idle_time_queue) && queue_pi_peek(context->idle_time_queue)->weight < t_start) {
        context->idle_time -= queue_pi_pop(context->idle_time_queue)->id;
    }

    uint64_t kernel_percentage = (context->kernel_time * 100) / window_length;
    uint64_t idle_percentage = (context->idle_time * 100) / window_length;

    context->active_task->registers[0] = kernel_percentage + 100 * idle_percentage;
}
