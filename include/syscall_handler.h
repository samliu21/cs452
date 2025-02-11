#ifndef _syscall_handler_h_
#define _syscall_handler_h_

#include "allocator.h"
#include "priority_queue.h"
#include "queue.h"
#include "task.h"
#include "uintmap.h"

typedef struct main_context_t {
    task_t* kernel_task;
    allocator_t* allocator;
    char* stack;
    uint64_t* n_tasks;
    priority_queue_t* scheduler;
    queue_t* tasks_waiting_for_send;
    queue_t* tasks_waiting_for_reply;
    queue_t* tasks_waiting_for_timer;
    queue_t* tasks_waiting_for_terminal;
    queue_t* tasks_waiting_for_marklin;
    task_t* active_task;
    uint32_t next_tick;
    uint64_t* total_time;
    uint64_t* kernel_time;
    uint64_t* idle_time;
} main_context_t;

void create_handler(main_context_t* context);
void my_tid_handler(main_context_t* context);
void my_parent_tid_handler(main_context_t* context);
void exit_handler(main_context_t* context);
void send_handler(main_context_t* context);
void receive_handler(main_context_t* context);
void reply_handler(main_context_t* context);
void await_event_handler(main_context_t* context);
void cpu_usage_handler(main_context_t* context);

#endif