.global init_vbar

init_vbar:
    adr x2, dummy_handler
    mrs x3, vbar_el1
    ldr x2, [x2]
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    str x2, [x3]
    add x3, x3, #16
    ret

dummy_handler:
    br x1