section .text
    global io_hlt, io_cli, io_sti, io_stihlt
    global io_in8, io_in16, io_in32
    global io_out8, io_out16, io_out32
    global io_load_eflags, io_store_eflags
    global load_gdtr, load_idtr
    global asm_inthandler20, asm_inthandler21, asm_inthandler27, asm_inthandler2c, asm_inthandler0d, asm_inthandler0c
    global load_cr0, store_cr0
    global load_tr, farjmp, farcall, start_app
    global memtest_sub
    global asm_hrb_api, asm_end_app
    extern inthandler20, inthandler21, inthandler27, inthandler2c, inthandler0d, inthandler0c, hrb_api

io_hlt:         ; void io_hlt()
    hlt
    ret

io_cli:         ; void io_cli()
    cli
    ret

io_sti:         ; void io_sti()
    sti
    ret

io_stihlt:      ; void io_stihlt()
    sti
    hlt
    ret

io_in8:         ; int io_in8(int port)
    mov     edx, [esp+4]    ; port
    mov     eax, 0
    in      al, dx
    ret

io_in16:         ; int io_in16(int port)
    mov     edx, [esp+4]    ; port
    mov     eax, 0
    in      ax, dx
    ret

io_in32:         ; int io_in32(int port)
    mov     edx, [esp+4]    ; port
    in      eax, dx
    ret

io_out8:        ; void io_out8(int port, int data)
    mov     edx, [esp+4]
    mov     al, [esp+8]
    out     dx, al
    ret
    
io_out16:        ; void io_out16(int port, int data)
    mov     edx, [esp+4]
    mov     eax, [esp+8]
    out     dx, ax
    ret
    
io_out32:        ; void io_out8(int port, int data)
    mov     edx, [esp+4]
    mov     eax, [esp+8]
    out     dx, eax
    ret

io_load_eflags: ; int io_load_eflags()
    pushfd
    pop     eax
    ret

io_store_eflags:; void io_store_eflags(int eflags)
    mov     eax, [esp+4]
    push    eax
    popfd
    ret
    
load_gdtr:     ; void load_gdtr(int limit, int addr);
    mov     ax, [esp+4]     ; limit
    mov     [esp+6], ax
    lgdt    [esp+6]
    ret

load_idtr:     ; void load_idtr(int limit, int addr);
    mov     ax, [esp+4]     ; limit
    mov     [esp+6], ax
    lidt    [esp+6]
    ret

load_cr0:       ; int load_cr0(void);
    mov     eax, cr0
    ret

store_cr0:      ; void store_cr0(int);
    mov     eax, [esp+4]
    mov     cr0, eax
    ret

%macro asm_inthandler 1
    push    es
    push    ds
	pushad
    mov     eax, esp
    push    eax
    mov     ax, ss
    mov     ds, ax
    mov     es, ax
    call    %1
    pop     eax
    popad
    pop     ds
    pop     es
    iretd
%endmacro

%macro asm_intexhandler 1
    push    es
    push    ds
	pushad
    mov     eax, esp
    push    eax
    mov     ax, ss
    mov     ds, ax
    mov     es, ax
    call    %1
    cmp     eax, 0
    jne     asm_end_app
    pop     eax
    popad
    pop     ds
    pop     es
    add     esp, 4
    iretd
%endmacro

asm_inthandler20:
    asm_inthandler inthandler20

asm_inthandler21:
    asm_inthandler inthandler21

asm_inthandler27:
    asm_inthandler inthandler27

asm_inthandler2c:
    asm_inthandler inthandler2c

asm_inthandler0d:
    asm_intexhandler inthandler0d

asm_inthandler0c:
    asm_intexhandler inthandler0c

memtest_sub:        ; unsigned int memtest_sub(unsigned int, unsigned int);
    push    edi
    push    esi
    push    ebx
    mov     esi, 0xaa55aa55     ; pat0 = 0xaa55aa55
    mov     edi, 0x55aa55aa     ; pat1 = 0x55aa55aa
    mov     eax, [esp+12+4]     ; i = start
mts_loop:
    mov     ebx, eax
    add     ebx, 0xffc          ; p= i+0xffc
    mov     edx, [ebx]          ; old = *p
    mov     [ebx], esi          ; *p = pat0
    xor     dword [ebx], 0xffffffff ; *p ^= 0xffffffff
    cmp     edi, [ebx]          ; if(*p != pat1) goto fin
    jne     mts_fin
    xor     dword [ebx], 0xffffffff ; *p ^= 0xffffffff
    cmp     esi, [ebx]          ; if(*p != pat0) goto fin
    jne     mts_fin
    mov     [ebx], edx          ; *p = old
    add     eax, 0x1000         ; i += 0x1000
    cmp     eax, [esp+12+8]     ; if(i<=end) goto mts_loop
    jbe     mts_loop
    pop     ebx
    pop     esi
    pop     edi
    ret
mts_fin:
    mov     [ebx], edx          ; *p = old
    pop     ebx
    pop     esi
    pop     edi
    ret
    
load_tr:    ; void load_tr(int);
    ltr     [esp+4]
    ret

farjmp:     ; void farjmp(int, int);
    jmp     far [esp+4]
    ret

farcall:    ; void farcall(int, int);
    call    far [esp+4]
    ret
    
asm_hrb_api:
    sti
    push    ds
    push    es
    pushad
    pushad    
    mov     ax, ss
    mov     ds, ax
    mov     es, ax
    call    hrb_api
    cmp     eax, 0
    jne     asm_end_app
    add     esp, 32
    popad
    pop     es
    pop     ds
    iretd

asm_end_app:
    mov     esp, [eax]
    mov     dword [eax+4], 0
    popad
    ret

start_app:  ; void start_app(int, int, int, int*)
    pushad
    mov     eax, [esp+36]
    mov     ecx, [esp+40]
    mov     edx, [esp+44]
    mov     ebx, [esp+48]
    mov     ebp, [esp+52]
    mov     [ebp], esp
    mov     [ebp+4], ss
    mov     es, bx
    mov     ds, bx
    mov     fs, bx
    mov     gs, bx

    or      ecx, 3
    or      ebx, 3
    push    ebx
    push    edx
    push    ecx
    push    eax
    retf
