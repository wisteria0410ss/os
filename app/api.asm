    global api_putchar, api_putstr, api_openwin
    global api_putstrwin, api_boxfillwin
    global api_initmalloc, api_malloc, api_free
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

api_putstrwin:
    push    edi
    push    esi
    push    ebp
    push    ebx
    mov     edx, 6
    mov     ebx, [esp+20]
    mov     esi, [esp+24]
    mov     edi, [esp+28]
    mov     eax, [esp+32]
    mov     ecx, [esp+36]
    mov     ebp, [esp+40]
    int     0x40
    pop     ebx
    pop     ebp
    pop     esi
    pop     edi
    ret

api_boxfillwin:
    push    edi
    push    esi
    push    ebp
    push    ebx
    mov     edx, 7
    mov     ebx, [esp+20]
    mov     eax, [esp+24]
    mov     ecx, [esp+28]
    mov     esi, [esp+32]
    mov     edi, [esp+36]
    mov     ebp, [esp+40]
    int     0x40
    pop     ebx
    pop     ebp
    pop     esi
    pop     edi
    ret

api_initmalloc:
    push    ebx
    mov     edx, 8
    mov     ebx, [cs:0x0020]
    mov     eax, ebx
    add     eax, 32*1024
    mov     ecx, [cs:0x0000]
    sub     ecx, eax
    int     0x40
    pop     ebx
    ret

api_malloc:
    push    ebx
    mov     edx, 9
    mov     ebx, [cs:0x0020]
    mov     ecx, [esp+8]
    int     0x40
    pop     ebx
    ret

api_free:
    push    ebx
    mov     edx, 10
    mov     ebx, [cs:0x0020]
    mov     eax, [esp+8]
    mov     ecx, [esp+12]
    int     0x40
    pop     ebx
    ret
    