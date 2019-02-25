    global api_putchar, api_putstr
    global api_end

section .text
api_putchar:
    mov     edx, 1
    mov     al, [esp+4]
    int     0x40
    ret

api_putstr:
    push    ebx
    mov     edx, 2
    mov     ebx, [esp+8]
    int     0x40
    pop     ebx
    ret

api_end:
    mov     edx, 4
    int     0x40
