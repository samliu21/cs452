#include "syscall.h"
#include "rpi.h"

int64_t syscall_handler(uint64_t esr) {
    uint64_t syscall_type = esr & SYNDROME_MASK;
    uart_printf(CONSOLE, "syscall type: %u\r\n", syscall_type);

    uint64_t registers[32];
    dump_registers((uint64_t)registers);
    for (int i = 0; i < 30; ++i) {
        uart_printf(CONSOLE, "register %d: %u\r\n", i, registers[i]);
    } // TODO: x2 and x3 aren't what we expect... investigate

    // syscall_type gets placed in x0, the call to enter_task should take this x0 register value as its own return value
    return syscall_type;
}
