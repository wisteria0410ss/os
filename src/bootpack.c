#include "bootpack.h"

void os_main(void){
	BootInfo *binfo = (BootInfo *)ADR_BOOTINFO;
	FIFO32 fifo, keycmd;
	char s[40];
	int fifobuf[128], keycmd_buf[32];
	Timer *timer;
	int x, y, mx, my;
	int cursor_x, cursor_c;
	int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	unsigned int memtotal;
	static char keytable[2][0x80] = {{
		0,	0,	'1','2','3','4','5','6','7','8','9','0','-','^',0,	0,	
		'Q','W','E','R','T','Y','U','I','O','P','@','[',0,	0,	'A','S',
		'D','F','G','H','J','K','L',';',':',0,	0,	']','Z','X','C','V',
		'B','N','M',',','.','/',0,	'*',0,	' ',0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	'7','8','9','-','4','5','6','+','1',
		'2','3','0','.',0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,'\\',	0,	0,	0,	0,	0,	0,	0,	0,	0,'\\',	0,	0
	},{
		0,	0,	'!','\"','#','$','%','&','\'','(',')',0  ,'=','~',0,	0,	
		'Q','W','E','R','T','Y','U','I','O','P','`','{',0,	0,	'A','S',
		'D','F','G','H','J','K','L','+','*',0,	0,	'}','Z','X','C','V',
		'B','N','M','<','>','?',0,	'*',0,	' ',0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	'7','8','9','-','4','5','6','+','1',
		'2','3','0','.',0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		0,	0,	0,	'_',0,	0,	0,	0,	0,	0,	0,	0,	0,	'|',0,	0
	}};

	MouseDec mdec;
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	ShtCtl *shtctl;
	Sheet *sht_back, *sht_mouse, *sht_win, *sht_cons, *sht;
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	Task *task_a, *task_cons;
	Console *cons;
	
	init_gdtidt();
	init_pic();
	io_sti();

	fifo32_init(&fifo, 128, fifobuf, 0);
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	init_pit();
	
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8);
	io_out8(PIC1_IMR, 0xef);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 0);

	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	init_screen(buf_back, binfo->scrnx, binfo->scrny);

	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *)memman_alloc_4k(memman, CONS_W*CONS_H);
	sheet_setbuf(sht_cons, buf_cons, CONS_W, CONS_H, -1);
	make_window8(buf_cons, CONS_W, CONS_H, "console", 0);
	make_textbox8(sht_cons, 8, 28, CONS_W-16, CONS_H-37, COL8_000000);
	task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64*1024) + 64*1024 - 12;
	task_cons->tss.eip = (int)&console_task;
	task_cons->tss.es  = 1*8;
	task_cons->tss.cs  = 2*8;
	task_cons->tss.ss  = 1*8;
	task_cons->tss.ds  = 1*8;
	task_cons->tss.fs  = 1*8;
	task_cons->tss.gs  = 1*8;
	*((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;
	*((int *)(task_cons->tss.esp + 8)) = memtotal;
	task_run(task_cons, 2, 2);

	*((int *) 0x0fe4) = (int)shtctl;

	sht_win = sheet_alloc(shtctl);
	buf_win = (unsigned char *)memman_alloc_4k(memman, 160*52);
	sheet_setbuf(sht_win, buf_win, 144, 52, -1);
	make_window8(buf_win, 144, 52, "task_a", 1);
	make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);

	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_cons, 32, 4);
	sheet_slide(sht_win,  binfo->scrnx - 256,  binfo->scrny - 144);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_cons, 1);
	sheet_updown(sht_win, 2);
	sheet_updown(sht_mouse, 3);

	putfonts8_asc(buf_back, binfo->scrnx, binfo->scrnx-8*12, binfo->scrny-46, COL8_000000, "Haribote OS.");
	putfonts8_asc(buf_back, binfo->scrnx, binfo->scrnx-8*12-1, binfo->scrny-47, COL8_FFFFFF, "Haribote OS.");
	sheet_refresh(sht_back, binfo->scrnx-8*12-1, binfo->scrny-47, binfo->scrnx, binfo->scrny);

	fifo32_push(&keycmd, KEYCMD_LED);
	fifo32_push(&keycmd, key_leds);

	while(1){
		if(fifo32_status(&keycmd) > 0 && keycmd_wait < 0){
			keycmd_wait = fifo32_pop(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if(fifo32_status(&fifo) == 0){
			task_sleep(task_a);
			io_sti();
		}else{
			int i = fifo32_pop(&fifo);
			io_sti();
			if(256 <= i && i < 512){
				if(i < 256 + 0x80) s[0] = keytable[key_shift!=0][i-256];
				else s[0] = 0;
				if('A' <= s[0] && s[0]<='Z'){
					if(((key_leds & 4)!=0) == (key_shift!=0)) s[0] += 0x20;
				}
				if(s[0] != 0){
					if(key_to == 0){	// task_A
						if(cursor_x < 128){
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					}else{	// console
						fifo32_push(&task_cons->fifo, s[0] + 256);
					}
				}
				if(i == 256 + 0x0e){		// bksp
					if(key_to == 0){
						if(cursor_x > 8){
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					}else{
						fifo32_push(&task_cons->fifo, 8 + 256);
					}
				}
				if(i == 256 + 0x0f){		// tab
					if(key_to == 0){
						key_to = 1;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
						cursor_c = -1;
						boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
						fifo32_push(&task_cons->fifo, 2);
					}else{
						key_to = 0;
						make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
						cursor_c = COL8_000000;
						fifo32_push(&task_cons->fifo, 3);
					}
					sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
					sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
				}
				if(i == 256 + 0x1c){		// enter
					if(key_to == 1){		// コンソールへ
						fifo32_push(&task_cons->fifo, 256 + 10);
					}
				}
				if(i == 256 + 0x3b && key_shift != 0 && task_cons->tss.ss0 != 0){	// shift + f1
					cons = (Console *) *((int *)0x0fec);
					cons_putstr(cons, "\nBreak(key): \n");
					io_cli();
					task_cons->tss.eax = (int)&(task_cons->tss.esp0);
					task_cons->tss.eip = (int)asm_end_app;
					io_sti(); 
				}
				if(i == 256 + 0x57 && shtctl->top > 2){		// F11
					sheet_updown(shtctl->sheets[1], shtctl->top-1);
				}
				if(i == 256 + 0x2a) key_shift |= 1;
				if(i == 256 + 0x36) key_shift |= 2;
				if(i == 256 + 0xaa) key_shift &= ~1;
				if(i == 256 + 0xb6) key_shift &= ~2;
				if(i == 256 + 0x3a){
					key_leds ^= 4;
					fifo32_push(&keycmd, KEYCMD_LED);
					fifo32_push(&keycmd, key_leds);
				}
				if(i == 256 + 0x45){
					key_leds ^= 2;
					fifo32_push(&keycmd, KEYCMD_LED);
					fifo32_push(&keycmd, key_leds);
				}
				if(i == 256 + 0x46){
					key_leds ^= 1;
					fifo32_push(&keycmd, KEYCMD_LED);
					fifo32_push(&keycmd, key_leds);
				}
				if(i == 256 + 0xfa) keycmd_wait = -1;
				if(i == 256 + 0xfe){
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				
				if(cursor_c >= 0) boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x+7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x+8, 44);
			}else if(512 <= i && i < 768){
				if(mouse_decode(&mdec, i - 512) != 0){
					mx += mdec.x;
					my += mdec.y;
					if(mx < 0) mx = 0;
					if(my < 0) my = 0;
					if(mx > binfo->scrnx-1) mx = binfo->scrnx - 1;
					if(my > binfo->scrny-1) my = binfo->scrny - 1;
					sheet_slide(sht_mouse, mx, my);	// refreshを含む
					if((mdec.btn & 0x01) != 0){
						for(int j=shtctl->top-1;j>0;j--){
							sht = shtctl->sheets[j];
							x = mx - sht->vx0;
							y = my - sht->vy0;
							if(0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize){
								if(sht->buf[y*sht->bxsize + x] != sht->col_transp){
									sheet_updown(sht, shtctl->top - 1);
									break;
								}
							}
						}
					}
				}
			}else if(i <= 1){
				if(i == 1){
					timer_init(timer, &fifo, 0);
					if(cursor_c >= 0) cursor_c = COL8_000000;
				}else{
					timer_init(timer, &fifo, 1);
					if(cursor_c >= 0) cursor_c = COL8_FFFFFF;
				}
				timer_settime(timer, 50);
				if(cursor_c >= 0){
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x+7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x+8, 44);
				}
			}
		}
	}
}
