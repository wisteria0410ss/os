bits 32
    mov     edx, 2
    mov     ebx, msg
    int     0x40
    retf
msg:
    db      "Hello", 10, 0
    