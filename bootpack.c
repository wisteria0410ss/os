#include "bootpack.h"

void os_main(void){
	BootInfo *binfo = (BootInfo *)ADR_BOOTINFO;
	FIFO32 fifo;
	char s[40];
	int fifobuf[128];
	Timer *timer[3];
	int mx, my;
	unsigned int memtotal, count = 0;
	MouseDec mdec;
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	ShtCtl *shtctl;
	Sheet *sht_back, *sht_mouse, *sht_win;
	unsigned char *buf_back, buf_mouse[256], *buf_win;

	init_gdtidt();
	init_pic();
	io_sti();

	fifo32_init(&fifo, 128, fifobuf);
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);

	init_pit();
	io_out8(PIC0_IMR, 0xf8);
	io_out8(PIC1_IMR, 0xef);

	const int timeout[3] = {1000, 300, 50}, timer_data[3] = {10, 3, 1};
	for(int i=0;i<3;i++){
		timer[i] = timer_alloc();
		timer_init(timer[i], &fifo, timer_data[i]);
		timer_settime(timer[i], timeout[i]);
	}
	
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win = (unsigned char *)memman_alloc_4k(memman, 160*52);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);
	init_screen(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);

	make_window8(buf_win, 160, 52, "counter");

	sheet_slide(sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);

	msprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
	int s_len = 0;
	msprintf(s, "memory %d MiB,   free: %d kiB%n", memtotal / (1024*1024), memman_total(memman) / 1024, &s_len);
	putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, s_len);
	putfonts8_asc(buf_back, binfo->scrnx, binfo->scrnx-8*12, binfo->scrny-46, COL8_000000, "Haribote OS.");
	putfonts8_asc(buf_back, binfo->scrnx, binfo->scrnx-8*12-1, binfo->scrny-47, COL8_FFFFFF, "Haribote OS.");
	sheet_refresh(sht_back, binfo->scrnx-8*12-1, binfo->scrny-47, binfo->scrnx, binfo->scrny);

	while(1){
		count++;

		io_cli();
		if(fifo32_status(&fifo) == 0){
			io_stihlt();
		}else{
			int i = fifo32_pop(&fifo);
			io_sti();
			if(256 <= i && i < 512){
				msprintf(s, "%02X", i - 256);
				putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
			}else if(512 <= i && i < 768){
				if(mouse_decode(&mdec, i - 512) != 0){
					msprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if(mdec.btn & 0x01) s[1] = 'L';
					if(mdec.btn & 0x02) s[3] = 'R';
					if(mdec.btn & 0x04) s[2] = 'C';
					putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);

					mx += mdec.x;
					my += mdec.y;
					if(mx < 0) mx = 0;
					if(my < 0) my = 0;
					if(mx > binfo->scrnx-1) mx = binfo->scrnx - 1;
					if(my > binfo->scrny-1) my = binfo->scrny - 1;
					msprintf(s, "(%3d, %3d)", mx, my);
					putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
					sheet_slide(sht_mouse, mx, my);	// refreshを含む
				}
			}else{
				switch(i){
				case 10:
					putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10 sec.", 7);
					msprintf(s, "%010d", count);
					putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);
					break;
				case 3:
					putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3 sec.", 6);
					count = 0;
					break;
				case 1:
					timer_init(timer[2], &fifo, 0);
					boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
					timer_settime(timer[2], 50);
					sheet_refresh(sht_back, 8, 96, 16, 112);
					break;
				case 0:
					timer_init(timer[2], &fifo, 1);
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
					timer_settime(timer[2], 50);
					sheet_refresh(sht_back, 8, 96, 16, 112);
					break;
				}
			}
		}
	}
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title){
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
	char c;
	boxfill8(buf, xsize, COL8_C6C6C6, 0,		0,		xsize-1, 0		);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,		1,		xsize-2, 1		);
	boxfill8(buf, xsize, COL8_C6C6C6, 0,		0,		0,		 ysize-1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,		1,		1, 		 ysize-2);
	boxfill8(buf, xsize, COL8_848484, xsize-2,	1,		xsize-2, ysize-2);
	boxfill8(buf, xsize, COL8_000000, xsize-1,	0,		xsize-1, ysize-1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,		2,		xsize-3, ysize-3);
	boxfill8(buf, xsize, COL8_000084, 3,		3,		xsize-4, 20		);
	boxfill8(buf, xsize, COL8_848484, 1,		ysize-2,xsize-2, ysize-2);
	boxfill8(buf, xsize, COL8_000000, 0,		ysize-1,xsize-1, ysize-1);
	putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
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