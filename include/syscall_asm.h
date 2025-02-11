#ifndef _syscall_h_
#define _syscall_h_

#include "common.h"
#include "rpi.h"
#include "util.h"

// syscall numbers
#define SYSCALL_CREATE 0
#define SYSCALL_MYTID 1
#define SYSCALL_MYPARENTTID 2
#define SYSCALL_YIELD 3
#define SYSCALL_EXIT 4
#define SYSCALL_SEND 5
#define SYSCALL_RECEIVE 6
#define SYSCALL_REPLY 7
#define SYSCALL_AWAIT_EVENT 8
#define SYSCALL_CPU_USAGE 9

#define INTERRUPT_CODE 100

#define USER_STACK_START (char*)0xFF00000

// syscall asm functions
extern int64_t create(uint64_t priority, func_t entry_point);
extern uint64_t my_tid();
extern uint64_t my_parent_tid();
extern void yield();
extern void exit();
extern int64_t send(uint64_t tid, const char* msg, uint64_t msglen, char* rp, uint64_t rplen);
extern int64_t receive(uint64_t* tid, char* msg, uint64_t msglen);
extern int64_t reply(uint64_t tid, const char* rp, uint64_t rplen);
extern int64_t await_event(uint64_t event_type);
extern uint64_t cpu_usage();

extern uint64_t debug_register();
extern void debug_set_registers(uint64_t i);
extern void debug_dump_registers(uint64_t* registers);

#endif
