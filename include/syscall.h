#ifndef _syscall_h_
#define _syscall_h_

#include "common.h"

#define SYNDROME_MASK 0xFFFF
#define SYSCALL_CREATE 0
#define SYSCALL_MYTID 1

// syscall asm functions
extern void create(uint64_t priority, uint64_t entry_point);
extern uint64_t my_tid();

// helper functions to test context switching
extern void default_registers_test();
extern void dump_registers(uint64_t registers);

#endif
