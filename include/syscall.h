#ifndef _syscall_h_
#define _syscall_h_

#include "common.h"

// syscall numbers
#define SYSCALL_CREATE 0
#define SYSCALL_MYTID 1
#define SYSCALL_MYPARENTTID 2
#define SYSCALL_YIELD 3
#define SYSCALL_EXIT 4

// syscall asm functions
extern int64_t create(uint64_t priority, uint64_t entry_point);
extern uint64_t my_tid();
extern uint64_t my_parent_tid();
extern void yield();
extern void exit();

extern void debug_set_registers();
extern void _debug_dump_registers(uint64_t registers);

// helper functions to test context switching
void debug_dump_registers()
{
    uint64_t registers[32];
    _debug_dump_registers((uint64_t)registers);
    for (int i = 0; i < 32; i++) {
        uart_printf(CONSOLE, "x%d: %u\r\n", i, registers[i]);
    }
}

#endif
