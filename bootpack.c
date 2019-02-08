#include "bootpack.h"

void os_main(void){
	BootInfo *binfo = (BootInfo *)ADR_BOOTINFO;
	char s[40];
	char mcursor[16*16];
	int mx, my;
	int mem;
	char keybuf[32], mousebuf[128];
	MouseDec mdec;

	init_gdtidt();
	init_pic();
	io_sti();

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);

	io_out8(PIC0_IMR, 0xf9);
	io_out8(PIC1_IMR, 0xef);

	init_keyboard();
	enable_mouse(&mdec);
	
	init_palette();
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

	putfonts8_asc(binfo->vram, binfo->scrnx, binfo->scrnx-8*12, binfo->scrny-46, COL8_000000, "Haribote OS.");
	putfonts8_asc(binfo->vram, binfo->scrnx, binfo->scrnx-8*12-1, binfo->scrny-47, COL8_FFFFFF, "Haribote OS.");

	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	msprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	init_mouse_cursor8(mcursor, COL8_008484);
	put_block8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	mem = memtest(0x00400000, 0xbfffffff) / (1024*1024);
	msprintf(s, "memory %d MiB", mem);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	while(1){
		io_cli();
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
			io_stihlt();
		}else{
			if(fifo8_status(&keyfifo) != 0){
				int i = fifo8_pop(&keyfifo);
				io_sti();
				msprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}else if(fifo8_status(&mousefifo) != 0){
				int i = fifo8_pop(&mousefifo);
				io_sti();
				if(mouse_decode(&mdec, i) != 0){
					msprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if(mdec.btn & 0x01) s[1] = 'L';
					if(mdec.btn & 0x02) s[3] = 'R';
					if(mdec.btn & 0x04) s[2] = 'C';

					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 8*15-1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx+15, my+15);
					mx += mdec.x;
					my += mdec.y;
					if(mx < 0) mx = 0;
					if(my < 0) my = 0;
					if(mx > binfo->scrnx-16) mx = binfo->scrnx - 16;
					if(my > binfo->scrny-16) my = binfo->scrny - 16;
					msprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
					put_block8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
				}
			}
		}
	}
}

unsigned int memtest(unsigned int start, unsigned int end){
	char flag486 = 0;
	unsigned int eflg, cr0, i;

	// 386? 486 or latar?
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if((eflg & EFLAGS_AC_BIT) != 0){ // 386ではAC-BITは勝手に0に戻る
		flag486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflg);

	if(flag486 != 0){
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;		// キャッシュ禁止
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if(flag486 != 0){
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;		// キャッシュ許可
		store_cr0(cr0);
	}

	return i;
}