botpak  equ 0x00280000      ; bootpackのロード先
dskcac  equ 0x00100000      ; ディスクキャッシュの場所
dskcac0 equ 0x00008000      ; ディスクキャッシュの場所（リアルモード）

cyls    equ 0x0ff0          ; ブートセクタが設定
leds    equ 0x0ff1          ; LED状態
vmode   equ 0x0ff2          ; 色数
scrnx   equ 0x0ff4          ; X解像度
scrny   equ 0x0ff6          ; Y解像度
vram    equ 0x0ff8          ; グラフィックバッファの開始番地

    org     0xc200

    mov     al, 0x13        ; VGAグラフィックス、320x200x8bit color
    mov     ah, 0x00        ; 画面モード切替
    int     0x10            ; ビデオBIOS呼出
    mov     byte [vmode], 8
    mov     word [scrnx], 320
    mov     word [scrny], 200
    mov     dword [vram], 0x000a0000

    mov     ah, 0x02
    int     0x16            ; キーボードBIOS呼出
    mov     [leds], al

; PICが一切の割り込みを受け付けないようにする
; AT互換機の仕様では、PICの初期化をするなら、
; こいつをCLI前にやっておかないと、たまにハングアップする
; PICの初期化はあとでやる
    mov     al, 0xff
    out     0x21, al
    nop                     ; OUT命令を連続させるとうまくいかない機種があるらしいので
    out     0xa1, al
    cli                     ; さらにCPUレベルでも割り込み禁止

; CPUから1MB以上のメモリにアクセスできるように、A20GATEを設定
    call    waitkbdout
    mov     al, 0xd1
    out     0x64, al
    call    waitkbdout
    mov     al, 0xdf        ; enable A20
    out     0x60, AL
    call    waitkbdout

; プロテクトモード移行
;[instrset "i486p"]         ; 486の命令まで使いたいという記述, nasmでは不要
    lgdt    [gdtr0]         ; 暫定GDTを設定
    mov     eax, cr0
    and     eax, 0x7fffffff ; bit31を0にする（ページング禁止のため）
    or      eax, 0x00000001 ; bit0を1にする（プロテクトモード移行のため）
    mov     cr0, eax
    jmp     pipelineflush

pipelineflush:
    mov     ax, 1*8         ; 読み書き可能セグメント32bit
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

; bootpackの転送
    mov     esi, bootpack   ; 転送元
    mov     edi, botpak     ; 転送先
    mov     ecx, 512*1024/4
    call    memcpy

; ついでにディスクデータも本来の位置へ転送
; まずはブートセクタから
    mov     esi, 0x7c00     ; 転送元
    mov     edi, dskcac     ; 転送先
    mov     ecx, 512/4
    call    memcpy

; 残り全部
    mov     esi, dskcac0+512; 転送元
    mov     edi, dskcac+512 ; 転送先
    mov     ecx, 0
    mov     cl, byte [cyls]
    imul    ecx, 512*18*2/4 ; シリンダ数からバイト数/4に変換
    sub     ecx, 512/4      ; IPLの分だけ差し引く
    call    memcpy

; asmheadでしなければいけないことは全部し終わったので、
; あとはbootpackに任せる

; bootpackの起動
    mov     ebx, botpak
    mov     ecx, [ebx+16]
    add     ecx, 3          ; ECX += 3;
    shr     ecx, 2          ; ECX /= 4;
    jz      skip            ; 転送するべきものがない
    mov     esi, [ebx+20]   ; 転送元
    add     esi, ebx
    mov     edi, [ebx+12]   ; 転送先
    call    memcpy

skip:
    mov     esp, [ebx+12]   ; スタック初期値
    jmp     dword 2*8:0x0000001b

waitkbdout:
    in      al, 0x64
    and     al, 0x02
    jnz     waitkbdout      ; ANDの結果が0でなければwaitkbdoutへ
    ret

memcpy:
    mov		eax, [esi]
    add		esi, 4
    mov		[edi], eax
    add		edi, 4
    sub		ecx, 1
    jnz		memcpy			; 引き算した結果が0でなければmemcpyへ
    ret
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

    align   16, db 0

gdt0:
    times   8 db 0          ; ヌルセレクタ
	dw      0xffff, 0x0000, 0x9200, 0x00cf  ; 読み書き可能セグメント32bit
	dw      0xffff, 0x0000, 0x9a28, 0x0047  ; 実行可能セグメント32bit（bootpack用）

	dw      0

gdtr0:
	dw      8*3-1
    dd      gdt0

    align   16, db 0

bootpack: