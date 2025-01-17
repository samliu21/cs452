.global init_vbar

init_vbar:
    ldr x0, =vbar
    msr vbar_el1, x0
    ret

vbar:
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
    .balign 0x80
    b dummy_handler
