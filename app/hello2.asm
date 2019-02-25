bits 32
    mov     edx, 2
    mov     ebx, msg
    int     0x40
    mov     edx, 4
    int     0x40
msg:
    db      "Hello", 10, 0
    