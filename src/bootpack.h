// asmhead.asm
typedef struct{
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
} BootInfo;
#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG  0x00100000

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
extern void asm_inthandler0d(void);
extern void asm_inthandler20(void);
extern void asm_inthandler21(void);
extern void asm_inthandler27(void);
extern void asm_inthandler2c(void);
extern void asm_hrb_api(void);
extern unsigned int memtest_sub(unsigned int, unsigned int);
extern void load_tr(int);
extern void farjmp(int, int);
extern void farcall(int, int);
extern void start_app(int, int, int, int, int *);

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

// graphic.c
void init_palette(void);
void set_palette(int, int, unsigned char*);
void init_screen(char*, int, int);
void init_mouse_cursor8(char*, char);
void put_block8(char*, int, int, int, int, int, char*, int);
void boxfill8(char*, int, unsigned char, int, int, int, int);
void putfont8(char*, int, int, int, unsigned char, char*);
void putfonts8_asc(char*, int, int, int, unsigned char, unsigned char*);
void putfonts8_asc_sht(Sheet *, int, int, int, int, char *, int);
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

// strcmp.c
int strcmp(const char *, const char *);
int starts_with(const char *, const char *);

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
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

// fifo.c
typedef struct{
	unsigned char *buf;
	int p, q, size, free, flags;
} FIFO8;
typedef struct{
	int *buf;
	int p, q, size, free, flags;
	struct Task *task;
} FIFO32;
void fifo8_init(FIFO8 *, int, unsigned char *);
int fifo8_push(FIFO8 *, unsigned char);
int fifo8_pop(FIFO8 *);
int fifo8_status(FIFO8 *);
void fifo32_init(FIFO32 *, int, int *, struct Task *);
int fifo32_push(FIFO32 *, int);
int fifo32_pop(FIFO32 *);
int fifo32_status(FIFO32 *);
#define FLAGS_OVERRUN	0x0001

// keyboard.c
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(FIFO32 *, int);
#define PORT_KEYSTA		0x0064
#define PORT_KEYCMD		0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KEYCMD_LED				0xed
#define KBC_MODE		0x47

// mouse.c
typedef struct{
	unsigned char buf[3], phase;
	int x, y, btn;
} MouseDec;
void inthandler2c(int *esp);
void enable_mouse(FIFO32 *, int, MouseDec *);
int mouse_decode(MouseDec *, unsigned char);
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

// timer.c
#define MAX_TIMER		500
#define TIMER_FLAGS_ALLOC	1
#define TIMER_FLAGS_USING	2
#define PIT_CTRL		0x0043
#define PIT_CNT0		0x0040
typedef struct Timer{
	struct Timer *next_timer;
	unsigned int timeout, flags;
	FIFO32 *fifo;
	int data;
} Timer;
typedef struct{
	unsigned int count, next;
	Timer *t0;
	Timer timers0[MAX_TIMER];
} TimerCtl;
extern TimerCtl timerctl;
void init_pit(void);
Timer *timer_alloc(void);
void timer_free(Timer *);
void timer_init(Timer *, FIFO32 *, int);
void timer_settime(Timer *, unsigned int);
void inthandler20(int *);

// mtask.c
#define MAX_TASKS 1000
#define MAX_TASKS_LV 100
#define MAX_TASKLEVELS 10
#define TASK_GDT0 3
typedef struct{
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
} TSS32;
typedef struct Task{
	int sel, flags;
	int level, priority;
	FIFO32 fifo;
	TSS32 tss;
}Task;
typedef struct TaskLevel{
	int runnning;
	int now;
	Task *tasks[MAX_TASKS_LV];
} TaskLevel;
typedef struct{
	int now_lv;
	char lv_change;
	TaskLevel level[MAX_TASKLEVELS];
	Task tasks0[MAX_TASKS];
}TaskCtl;
extern Timer *task_timer;
Task *task_init(MemMan *);
Task *task_alloc(void);
void task_run(Task *, int, int);
void task_switch(void);
void task_sleep(Task *);
Task *task_now(void);
void task_add(Task *);
void task_remove(Task *);
void task_switchsub(void);
void task_idle(void);

// hankaku.c
char *get_fontdata(void);

// window.c
void make_window8(unsigned char *, int, int, char *, char);
void make_wtitle8(unsigned char *, int, char *, char);
void make_textbox8(Sheet *, int, int, int, int, int);

// console.c
typedef struct{
	Sheet *sht;
	int cur_x, cur_y, cur_c;
} Console;
#define CONS_W 960
#define CONS_H 640
void console_task(Sheet *, unsigned int);
void cons_putchar(Console *, int, char);
void cons_newline(Console *);
void cons_runcmd(char *, Console *, int *, unsigned int);
void cmd_mem(Console *, unsigned int);
void cmd_cls(Console *);
void cmd_dir(Console *);
void cmd_type(Console *, int *, char *);
int cmd_app(Console *, int *, char *);
void cons_putstr(Console *, char *);
void cons_nputstr(Console *, char *, int);
int *hrb_api(int, int, int, int, int, int, int, int);
int *inthandler0d(int *);

// file.c
typedef struct{
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
} FileInfo;
void file_readfat(int *, unsigned char *);
void file_loadfile(int, int, char *, int *, char *);
FileInfo *file_search(char *, FileInfo *, int);

// bootpack.c
