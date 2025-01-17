#ifndef _syscall_h_
#define _syscall_h_

#include "common.h"

#define SYNDROME_MASK 0xFFFF

#define SYSCALL_MYTID 1

int64_t syscall_handler(uint64_t esr);

extern void default_registers_test();

extern void dump_registers(uint64_t registers);

#endif
