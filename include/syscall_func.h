#ifndef _syscall_func_h_
#define _syscall_func_h_

#include "syscall_asm.h"

int64_t reply_uint(uint64_t tid, uint64_t rp);
int64_t reply_null(uint64_t tid);
void debug_print_registers();

#endif
