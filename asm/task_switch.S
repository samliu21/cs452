#include "asm.include"
.global enter_task
.global syscall_handler
.global interrupt_handler

enter_task:
	// store user task, kernel task on kernel stack
    stp x1, x0, [sp, #-16]!

	# store kernel registers
    stp x0, x1, [x0]
    add x0, x0, #16
    stp x2, x3, [x0], #16
    stp x4, x5, [x0], #16
    stp x6, x7, [x0], #16
    stp x8, x9, [x0], #16
    stp x10, x11, [x0], #16
    stp x12, x13, [x0], #16
    stp x14, x15, [x0], #16
    stp x16, x17, [x0], #16
    stp x18, x19, [x0], #16
    stp x20, x21, [x0], #16
    stp x22, x23, [x0], #16
    stp x24, x25, [x0], #16
    stp x26, x27, [x0], #16
    stp x28, x29, [x0], #16
    str x30, [x0]

	# load user's ELR, SP, SPSR from user
    ldr x0, [x1, #248]
    msr elr_el1, x0
    ldr x0, [x1, #256]
    msr sp_el0, x0
    ldr x0, [x1, #264]
    msr spsr_el1, x0

	// load user registers (except x1)
    ldr x0, [x1], #16
    ldr x2, [x1], #8
    ldp x3, x4, [x1], #16
    ldp x5, x6, [x1], #16
    ldp x7, x8, [x1], #16
    ldp x9, x10, [x1], #16
    ldp x11, x12, [x1], #16
    ldp x13, x14, [x1], #16
    ldp x15, x16, [x1], #16
    ldp x17, x18, [x1], #16
    ldp x19, x20, [x1], #16
    ldp x21, x22, [x1], #16
    ldp x23, x24, [x1], #16
    ldp x25, x26, [x1], #16
    ldp x27, x28, [x1], #16
    ldp x29, x30, [x1], #16

	// load x1
    ldr x1, [sp]
    add x1, x1, #8
    ldr x1, [x1]

    eret

.macro restore_kernel
	// store the current user x1 register
    str x1, [sp, #-8]!
    // load the user's task descriptor
    ldr x1, [sp, #8]

    // save user registers (except x1)
    str x0, [x1], #16
    str x2, [x1], #8
    stp x3, x4, [x1], #16
    stp x5, x6, [x1], #16
    stp x7, x8, [x1], #16
    stp x9, x10, [x1], #16
    stp x11, x12, [x1], #16
    stp x13, x14, [x1], #16
    stp x15, x16, [x1], #16
    stp x17, x18, [x1], #16
    stp x19, x20, [x1], #16
    stp x21, x22, [x1], #16
    stp x23, x24, [x1], #16
    stp x25, x26, [x1], #16
    stp x27, x28, [x1], #16
    stp x29, x30, [x1], #16

	// store ELR, SP, SPSR
    ldr x1, [sp, #8]
    mrs x2, elr_el1
    str x2, [x1, #248]
    mrs x2, sp_el0
    str x2, [x1, #256]
    mrs x2, spsr_el1
    str x2, [x1, #264]

	// store x1
	ldr x1, [sp], #8 // x1 is the user's x1 register
    ldr x0, [sp], #8 // x0 is the user's task descriptor
    str x1, [x0, #8]

	// load kernel registers
    ldr x0, [sp], #8
    add x0, x0, #8
    ldp x1, x2, [x0], #16
    ldp x3, x4, [x0], #16
    ldp x5, x6, [x0], #16
    ldp x7, x8, [x0], #16
    ldp x9, x10, [x0], #16
    ldp x11, x12, [x0], #16
    ldp x13, x14, [x0], #16
    ldp x15, x16, [x0], #16
    ldp x17, x18, [x0], #16
    ldp x19, x20, [x0], #16
    ldp x21, x22, [x0], #16
    ldp x23, x24, [x0], #16
    ldp x25, x26, [x0], #16
    ldp x27, x28, [x0], #16
    ldp x29, x30, [x0], #16
.endm


syscall_handler:
    restore_kernel
    mrs x0, esr_el1
    ret

interrupt_handler:
    restore_kernel
    mov x0, #100
    // ldr x0, [x0]
    ret

//iar_address:
  //  .word 0xff840000
