// asmhead.asm
typedef struct{
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
} BootInfo;
#define ADR_BOOTINFO 0x00000ff0

// func.asm
extern void io_hlt(void);
extern void io_cli(void);
extern void io_sti(void);
extern void io_stihlt(void);
extern int io_in8(int);
extern int io_in16(int);
extern int io_in32(int);
extern void io_out8(int, int);
extern void io_out16(int, int);
extern void io_out32(int, int);
extern int io_load_eflags(void);
extern void io_store_eflags(int);
extern void load_gdtr(int, int);
extern void load_idtr(int, int);
extern int load_cr0(void);
extern void store_cr0(int);
extern void asm_inthandler21(void);
extern void asm_inthandler27(void);
extern void asm_inthandler2c(void);
extern unsigned int memtest_sub(unsigned int, unsigned int);

// graphic.c
void init_palette(void);
void set_palette(int, int, unsigned char*);
void init_screen(char*, int, int);
void init_mouse_cursor8(char*, char);
void put_block8(char*, int, int, int, int, int, char*, int);
void boxfill8(char*, int, unsigned char, int, int, int, int);
void putfont8(char*, int, int, int, unsigned char, char*);
void putfonts8_asc(char*, int, int, int, unsigned char, unsigned char*);
#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

// sprintf.c
int msprintf(char*, const char*, ...);

// dsctbl.c
typedef struct{
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
} SegmentDescriptor;
typedef struct{
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
} GateDescriptor;

void init_gdtidt(void);
void set_segmdesc(SegmentDescriptor *, unsigned int, int, int);
void set_gatedesc(GateDescriptor *, int, int, int);
#define ADR_IDT     0x0026f800
#define LIMIT_IDT   0x000007ff
#define ADR_GDT     0x00270000
#define LIMIT_GDT   0x0000ffff
#define ADR_BOTPAK  0x00280000
#define LIMIT_BOTPAK    0x0007ffff
#define AR_DATA32_RW    0x4092
#define AR_CODE32_ER    0x409a
#define AR_INTGATE32	0x008e

// fifo.c
typedef struct{
	unsigned char *buf;
	int p, q, size, free, flags;
} FIFO8;
void fifo8_init(FIFO8 *, int, unsigned char *);
int fifo8_push(FIFO8 *, unsigned char);
int fifo8_pop(FIFO8 *);
int fifo8_status(FIFO8 *);
#define FLAGS_OVERRUN	0x0001

// keyboard.c
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(void);
extern FIFO8 keyfifo;
#define PORT_KEYSTA		0x0064
#define PORT_KEYCMD		0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE		0x47

// mouse.c
typedef struct{
	unsigned char buf[3], phase;
	int x, y, btn;
} MouseDec;
void inthandler2c(int *esp);
void enable_mouse(MouseDec *);
int mouse_decode(MouseDec *, unsigned char);
extern FIFO8 mousefifo;
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE	0xf4

// int.c
void init_pic(void);
void inthandler27(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
#define PORT_KEYDAT		0x0060

// memory.c
#define MEMMAN_FREES	4090
typedef struct{
	unsigned int addr, size;
} FreeInfo;
typedef struct{
	int frees, maxfrees, lostsize, losts;
	FreeInfo free[MEMMAN_FREES];
} MemMan;
unsigned int memtest(unsigned int, unsigned int);
void memman_init(MemMan *);
unsigned int memman_total(MemMan *);
unsigned int memman_alloc(MemMan *, unsigned int);
int memman_free(MemMan *, unsigned int, unsigned int);
unsigned int memman_alloc_4k(MemMan *, unsigned int);
int memman_free_4k(MemMan *, unsigned int, unsigned int);
#define EFLAGS_AC_BIT	0x00040000
#define CR0_CACHE_DISABLE	0x60000000
#define MEMMAN_ADDR		0x003c0000

// sheet.c
#define MAX_SHEETS		256
#define SHEET_USE		1
typedef struct{
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_transp, height, flags;
	struct ShtCtl *ctl;
} Sheet;
typedef struct ShtCtl{
	unsigned char *vram, *map;
	int xsize, ysize, top;
	Sheet *sheets[MAX_SHEETS];
	Sheet sheets0[MAX_SHEETS];
} ShtCtl;
ShtCtl *shtctl_init(MemMan *, unsigned char *, int, int);
Sheet *sheet_alloc(ShtCtl *);
void sheet_setbuf(Sheet *, unsigned char *, int, int, int);
void sheet_updown(Sheet *, int);
void sheet_refresh(Sheet *, int, int, int, int);
void sheet_refreshsub(ShtCtl *, int, int, int, int, int, int);
void sheet_slide(Sheet *, int, int);
void sheet_refreshmap(ShtCtl *, int, int, int, int, int);
void sheet_free(Sheet *);

// bootpack.c
void make_window8(unsigned char *, int, int, char *);