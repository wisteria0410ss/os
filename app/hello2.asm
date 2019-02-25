bits 32
    global  app_main

section .text
app_main:
    mov     edx, 2
    mov     ebx, msg
    int     0x40
    mov     edx, 4
    int     0x40
    
section .data
msg:
    db      "Hello", 10, 0
    