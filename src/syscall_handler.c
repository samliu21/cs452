#include "syscall_handler.h"
#include "rpi.h"
#include <stdlib.h>

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
        pq_add(context->scheduler, new_task);
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
        queue_delete(context->tasks_waiting_for_send, receiver);
        pq_add(context->scheduler, receiver);
        receiver->state = READY;

        // block sender
        queue_add(context->tasks_waiting_for_reply, context->active_task);
        context->active_task->state = REPLYWAIT;
    } else { // the receiver has NOT requested a message
        receiver = allocator_get_task(context->allocator, context->active_task->registers[0]);
        if (receiver == NULL) {
            context->active_task->registers[0] = -1;
        } else {
            ASSERT(receiver != NULL, "receiver not allocated");
            queue_add(&(receiver->senders_queue), context->active_task);
            context->active_task->state = SENDWAIT;
        }
    }
}

void receive_handler(main_context_t* context)
{
    if (queue_empty(&context->active_task->senders_queue)) { // no senders, wait
        context->active_task->state = RECEIVEWAIT;
        queue_add(context->tasks_waiting_for_send, context->active_task);
    } else { // there is a sender, fulfill their request
        task_t* sender = queue_pop(&context->active_task->senders_queue);
        ASSERT(sender->state == SENDWAIT, "blocked sender is not in send wait state");

        send_receive(sender, context->active_task);

        // unblock sender
        queue_add(context->tasks_waiting_for_reply, sender);
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
    queue_delete(context->tasks_waiting_for_reply, sender);
    pq_add(context->scheduler, sender);
    sender->state = READY;
}
