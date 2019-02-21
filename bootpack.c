#include "bootpack.h"

void os_main(void){
	BootInfo *binfo = (BootInfo *)ADR_BOOTINFO;
	FIFO32 fifo, keycmd;
	char s[40];
	int fifobuf[128], keycmd_buf[32];
	Timer *timer;
	int mx, my;
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
	Sheet *sht_back, *sht_mouse, *sht_win, *sht_cons;
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	Task *task_a, *task_cons;
	
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
	sheet_slide(sht_win,  64,  56);
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
					if((mdec.btn & 0x01) != 0)sheet_slide(sht_win, mx-80, my-8);
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

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act){
	boxfill8(buf, xsize, COL8_C6C6C6, 0,		0,		xsize-1, 0		);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,		1,		xsize-2, 1		);
	boxfill8(buf, xsize, COL8_C6C6C6, 0,		0,		0,		 ysize-1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,		1,		1, 		 ysize-2);
	boxfill8(buf, xsize, COL8_848484, xsize-2,	1,		xsize-2, ysize-2);
	boxfill8(buf, xsize, COL8_000000, xsize-1,	0,		xsize-1, ysize-1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,		2,		xsize-3, ysize-3);
	boxfill8(buf, xsize, COL8_848484, 1,		ysize-2,xsize-2, ysize-2);
	boxfill8(buf, xsize, COL8_000000, 0,		ysize-1,xsize-1, ysize-1);
	make_wtitle8(buf, xsize, title, act);

	return;
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act){
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c, tc, tbc;
	if(act != 0){
		tc = COL8_FFFFFF;
		tbc = COL8_000084;
	}else{
		tc = COL8_C6C6C6;
		tbc = COL8_848484;
	}
	boxfill8(buf, xsize, tbc,		  3,		3,		xsize-4, 20		);
	putfonts8_asc(buf, xsize, 24, 4, tc, title);
	for(y=0;y<14;y++){
		for(x=0;x<16;x++){
			c = closebtn[y][x];
			switch(c){
				case '@':
					c = COL8_000000;
					break;
				case '$':
					c = COL8_848484;
					break;
				case 'Q':
					c = COL8_C6C6C6;
					break;
				default:
					c = COL8_FFFFFF;
			}
			buf[(5+y)*xsize + (xsize-21+x)] = c;
		}
	}

	return;
}

void make_textbox8(Sheet *sht, int x0, int y0, int sx, int sy, int c){
	int x1 = x0+sx, y1 = y0+sy;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0-2, y0-3, x1+1, y0-3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0-3, y0-3, x0-3, y1+1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0-3, y1+2, x1+1, y1+2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1+2, y0-3, x1+2, y1+2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0-1, y0-2, x1+0, y0-2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0-2, y0-2, x0-2, y1+0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0-2, y1+1, x1+0, y1+1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1+1, y0-2, x1+1, y1+1);
	boxfill8(sht->buf, sht->bxsize, c          , x0-1, y0-1, x1+0, y1+0);

	return;
}

void set490(FIFO32 *fifo, int mode){
	int i;
	Timer *timer;
	if(mode != 0){
		for(i=0;i<490;i++){
			timer = timer_alloc();
			timer_init(timer, fifo, 1024+i);
			timer_settime(timer, 100*60*60*24*50 + i*100);
		}
	}
	return;
}

void console_task(Sheet *sheet, unsigned int memtotal){
	Timer *timer;
	Task *task = task_now();

	int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
	char s[30], cmdline[30];
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	FileInfo *finfo = (FileInfo *)(ADR_DISKIMG + 0x002600);

	fifo32_init(&task->fifo, 128, fifobuf, task);
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
	
	putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

	while(1){
		io_cli();
		if(fifo32_status(&task->fifo) == 0){
			task_sleep(task);
			io_sti();
		}else{
			i = fifo32_pop(&task->fifo);
			io_sti();
			if(i <= 1){
				if(i != 0){
					timer_init(timer, &task->fifo, 0);
					if(cursor_c >= 0) cursor_c = COL8_FFFFFF;
				}else{
					timer_init(timer, &task->fifo, 1);
					if(cursor_c >= 0) cursor_c = COL8_000000;
				}
				timer_settime(timer, 50);
			}
			if(i == 2) cursor_c = COL8_FFFFFF;
			if(i == 3){
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
				cursor_c = -1;
			}
			if(256 <= i && i < 512){
				if(i == 8+256){		// bksp
					if(cursor_x > 16){
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
						cursor_x -= 8;
					}
				}else if(i == 10 + 256){	// enter
					putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
					cmdline[cursor_x/8 - 2] = 0;
					cursor_y = cons_newline(cursor_y, sheet);
					if(strcmp(cmdline, "mem") == 0){
						// mem コマンド
						int len;
						len = msprintf(s, "Total  %d MiB", memtotal / (1024*1024));
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, len);
						cursor_y = cons_newline(cursor_y, sheet);
						len = msprintf(s, "Free   %d kiB", memman_total(memman) / 1024);
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, len);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}else if(strcmp(cmdline, "clear") == 0){
						for(int y=28;y<CONS_H-8;y++){
							for(int x=8;x<CONS_W-7;x++){
								sheet->buf[x + y*sheet->bxsize] = COL8_000000;
							}
						}
						sheet_refresh(sheet, 8, 28, CONS_W-7, CONS_H-8);
						cursor_y = 28;
					}else if(strcmp(cmdline, "dir") == 0){
						for(int x=0;x<224;x++){
							if(finfo[x].name[0] == 0x00) break;
							if(finfo[x].name[0] != 0xe5){
								if((finfo[x].type & 0x18) == 0){
									int len = msprintf(s, "%.8s.%.3s  %d", finfo[x].name, finfo[x].ext, finfo[x].size);
									putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, len);
									cursor_y = cons_newline(cursor_y, sheet);
								}
							}
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}else if(cmdline[0] != 0){
						int len;
						len = msprintf(s, "command \'%s\' not found.", cmdline);
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, len);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}

					putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
					cursor_x = 16;
				}else{
					if(cursor_x < CONS_W - 16){
						s[0] = i-256;
						s[1] = 0;
						cmdline[cursor_x/8 - 2] = i-256;
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
						cursor_x += 8;
					}
				}
			}
			if(cursor_c >= 0) boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
			sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
		}
	}
}

int cons_newline(int cursor_y, Sheet *sheet){
	if(cursor_y + 32 < CONS_H - 8) cursor_y += 16;
	else{
		for(int y=28;y<CONS_H-24;y++){
			for(int x=8;x<=CONS_W-8;x++){
				sheet->buf[x + y*sheet->bxsize] = sheet->buf[x + (y+16)*sheet->bxsize];
			}
		}
		for(int y=CONS_H-24;y<CONS_H-8;y++){
			for(int x=8;x<=CONS_W-8;x++){
				sheet->buf[x + y*sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, CONS_W-7, CONS_H-9);
	}

	return cursor_y;
}