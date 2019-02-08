#include "bootpack.h"

void os_main(void){
	BootInfo *binfo = (BootInfo *)ADR_BOOTINFO;
	char s[40];
	char mcursor[16*16];
	int mx, my;
	unsigned int memtotal;
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	char keybuf[32], mousebuf[128];
	MouseDec mdec;
	ShtCtl *shtctl;
	Sheet *sht_back, *sht_mouse;
	unsigned char *buf_back, buf_mouse[256];

	init_gdtidt();
	init_pic();
	io_sti();

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);

	io_out8(PIC0_IMR, 0xf9);
	io_out8(PIC1_IMR, 0xef);

	init_keyboard();
	enable_mouse(&mdec);
	
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_screen(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);

	sheet_slide(shtctl, sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(shtctl, sht_mouse, mx, my);
	sheet_updown(shtctl, sht_back, 0);
	sheet_updown(shtctl, sht_mouse, 1);

	msprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	msprintf(s, "memory %d MiB,   free: %d kiB", memtotal / (1024*1024), memman_total(memman) / 1024);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	putfonts8_asc(buf_back, binfo->scrnx, binfo->scrnx-8*12, binfo->scrny-46, COL8_000000, "Haribote OS.");
	putfonts8_asc(buf_back, binfo->scrnx, binfo->scrnx-8*12-1, binfo->scrny-47, COL8_FFFFFF, "Haribote OS.");
	sheet_refresh(shtctl);

	while(1){
		io_cli();
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
			io_stihlt();
		}else{
			if(fifo8_status(&keyfifo) != 0){
				int i = fifo8_pop(&keyfifo);
				io_sti();
				msprintf(s, "%02X", i);
				boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}else if(fifo8_status(&mousefifo) != 0){
				int i = fifo8_pop(&mousefifo);
				io_sti();
				if(mouse_decode(&mdec, i) != 0){
					msprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if(mdec.btn & 0x01) s[1] = 'L';
					if(mdec.btn & 0x02) s[3] = 'R';
					if(mdec.btn & 0x04) s[2] = 'C';

					boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 8*15-1, 31);
					putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx+15, my+15);
					mx += mdec.x;
					my += mdec.y;
					if(mx < 0) mx = 0;
					if(my < 0) my = 0;
					if(mx > binfo->scrnx-16) mx = binfo->scrnx - 16;
					if(my > binfo->scrny-16) my = binfo->scrny - 16;
					msprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
					sheet_slide(shtctl, sht_mouse, mx, my);	// refreshを含む
				}
			}
		}
	}
}
