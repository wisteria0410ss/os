cyls equ 10                 ; 読み込むシリンダ数

    org     0x7c00          ; プログラムが読み込まれる位置

    jmp     entry           
    db      0x90
    db      "HARIBOTE"      ; ブートセクタ名(8 byte)
    dw      512             ; 1 セクタの大きさ(512)
    db      1               ; クラスタの大きさ(1)
    dw      1               ; FATの始点(普通は1)
    db      2               ; FATの数(2)
    dw      224             ; ルートディレクトリ領域の大きさ(普通224)
    dw      2880            ; ドライブの大きさ(2880 セクタ)
    db      0xf0            ; メディアのタイプ(0xf0)
    dw      9               ; FAT領域の長さ(9 セクタ)
    dw      18              ; トラックあたりのセクタ数(18 セクタ)
    dw      2               ; ヘッドの数(2)
    dd      0               ; パーティションを使っていないので 0
    dd      2880            ; ドライブの大きさ(2880 セクタ)
    db      0, 0, 0x29      ; こうする
    dd      0xffffffff      ; ボリュームシリアル番号？
    db      "HARIBOTE-OS"   ; ディスク名(11 byte)
    db      "FAT12   "      ; フォーマット名(8 byte)
    times   18  db  0       ; 18 byteゼロ埋め

entry:
    mov     ax, 0           ; レジスタ初期化
    mov     ss, ax
    mov     sp, 0x7c00
    mov     ds, ax

; ディスク読み込み
    mov     ax, 0x0820
    mov     es, ax
    mov     ch, 0           ; シリンダ番号 & 0xff = 0 & 0xff
    mov     dh, 0           ; ヘッド番号 0
    mov     cl, 2           ; セクタ番号(bit0-5) | (シリンダ番号 & 0x300) >> 2 = 2 | 0

readloop:
    mov     si, 0           ; 失敗回数保持

retry:
    mov     ah, 0x02        ; 読み込み
    mov     al, 1           ; 処理セクタ数
    mov     bx, 0           
    mov     dl, 0x00        ; ドライブ番号 Aドライブ
    int     0x13            ; ディスクBIOS 呼出
    jnc     next
    add     si, 1
    cmp     si, 5
    jae     error
    mov     ah, 0x00        ; リセット
    mov     dl, 0x00        ; ドライブ番号 Aドライブ
    int     0x13            ; ディスクBIOS 呼出
    jmp     retry

next:
    mov     ax, es          ; es に 512/16 = 0x20 を加算
    add     ax, 512/16
    mov     es, ax          ; アドレスが 0x200 進んだ
    add     cl, 1           ; 次のセクタへ
    cmp     cl, 18
    jbe     readloop        ; セクタ番号 <= 18 なら読む
    mov     cl, 1           ; セクタ番号(1-indexed)を 1 に
    add     dh, 1           ; ヘッド番号に 1 加算
    cmp     dh, 2
    jb      readloop        ; ヘッド番号 < 2 なら読む
    mov     dh, 0           ; ヘッド番号を 0 に
    add     ch, 1           ; 次のシリンダへ
    cmp     ch, cyls
    jb      readloop        ; シリンダ番号 < cyls なら読む

    mov     [0x0ff0], ch    ; IPLがどこまで読んだのかをメモ
    jmp     0xc200          ; haribote.sys の実行
fin:
    hlt
	jmp     fin

error:
    mov     si, msg

putloop:
    mov     al, [si]
    add     si, 1
    cmp     al, 0
    je      fin
    mov     ah, 0x0e        ; 1文字表示
    mov     bx, 15          ; カラーコード
    int     0x10            ; ビデオBIOS 呼出
    jmp     putloop

msg:
    db      0x0a, 0x0a      ; 0x0a: 改行
	db      "load error"
    db      0x0a
    db      0

    times   0x1fe-($-$$) db 0
    DB      0x55, 0xaa
