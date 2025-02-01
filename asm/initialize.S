.global init_vbar

init_vbar:
    ldr x0, =vbar
    msr vbar_el1, x0
    ret

.balign 0x800
vbar:
    b dummy_handler // SP0
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80 // current EL
    mrs x0, esr_el1
    b synchronous_kernel_error
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80 // lower EL (64-bit)
    b syscall_handler
    .balign 0x80
    b interrupt_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80 // lower EL (32-bit)
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
