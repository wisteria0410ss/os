bits 32
    global  app_main
    
app_main:
    mov     ecx, msg
    mov     edx, 1
putloop:
    mov     al, [cs:ecx]
    cmp     al, 0
    je      fin
    int     0x40
    add     ecx, 1
    jmp     putloop
fin:
    mov     edx, 4
    int     0x40
msg:
    db      "Hello", 10, 0
    