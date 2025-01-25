#include "syscall_func.h"

int64_t reply_num(uint64_t tid, uint64_t rp)
{
    char buf[32];
    ui2a(rp, 10, buf);
    return reply(tid, buf, strlen(buf));
}

int64_t reply_empty(uint64_t tid)
{
    return reply(tid, NULL, 0);
}

// helper functions to test context switching
void debug_print_registers()
{
    uint64_t registers[32];
    debug_dump_registers(registers);
    for (int i = 0; i < 32; i++) {
        uart_printf(CONSOLE, "x%d: %u\r\n", i, registers[i]);
    }
}
