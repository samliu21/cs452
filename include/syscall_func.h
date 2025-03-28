#ifndef _syscall_func_h_
#define _syscall_func_h_

#include "syscall_asm.h"

int64_t send_empty(uint64_t tid);
int64_t receive_empty(uint64_t* tid);
int64_t reply_char(uint64_t tid, char rp);
int64_t reply_num(uint64_t tid, uint64_t rp);
int64_t reply_empty(uint64_t tid);
void debug_print_registers();

#endif
