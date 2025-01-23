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

// syscall asm functions
extern int64_t create(uint64_t priority, func_t entry_point);
extern uint64_t my_tid();
extern uint64_t my_parent_tid();
extern void yield();
extern void exit();
extern int64_t send(uint64_t tid, const char* msg, uint64_t msglen, char* rp, uint64_t rplen);
extern int64_t receive(uint64_t* tid, char* msg, uint64_t msglen);
extern int64_t reply(uint64_t tid, const char* rp, uint64_t rplen);

int64_t reply_uint(uint64_t tid, uint64_t rp)
{
    char buf[32];
    ui2a(rp, 10, buf);
    return reply(tid, buf, strlen(buf));
}

int64_t reply_null(uint64_t tid)
{
    return reply(tid, NULL, 0);
}

extern void debug_set_registers(uint64_t i);
extern void debug_dump_registers(uint64_t* registers);

// helper functions to test context switching
void debug_print_registers()
{
    uint64_t registers[32];
    debug_dump_registers(registers);
    for (int i = 0; i < 32; i++) {
        uart_printf(CONSOLE, "x%d: %u\r\n", i, registers[i]);
    }
}

#endif
