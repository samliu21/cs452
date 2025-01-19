#ifndef _syscall_h_
#define _syscall_h_

#include "common.h"

// syscall numbers
#define SYSCALL_CREATE 0
#define SYSCALL_MYTID 1
#define SYSCALL_MYPARENTTID 2

// syscall asm functions
extern void create(uint64_t priority, uint64_t entry_point);
extern uint64_t my_tid();
extern uint64_t my_parent_tid();

// helper functions to test context switching
extern void debug_set_registers();
extern void debug_dump_registers(uint64_t registers);

#endif
