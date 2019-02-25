    global api_putchar, api_putstr, api_openwin
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

api_openwin:
    push    edi
    push    esi
    push    ebx
    mov     edx, 5
    mov     ebx, [esp+16]
    mov     esi, [esp+20]
    mov     edi, [esp+24]
    mov     eax, [esp+28]
    mov     ecx, [esp+32]
    int     0x40
    pop     ebx
    pop     esi
    pop     edi
    ret
