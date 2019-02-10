#include "bootpack.h"

void init_palette(){
	static unsigned char table_rgb[3*16] = {
		0x00, 0x00, 0x00,	// 0: black
		0xff, 0x00, 0x00,	// 1: red
		0x00, 0xff, 0x00,	// 2: green
		0xff, 0xff, 0x00,	// 3: yellow
		0x00, 0x00, 0xff,	// 4: blue
		0xff, 0x00, 0xff,	// 5: magenta
		0x00, 0xff, 0xff,	// 6: cyan
		0xff, 0xff, 0xff,	// 7: white
		0xc6, 0xc6, 0xc6,	// 8: light gray
		0x84, 0x00, 0x00,	// 9: dark red
		0x00, 0x84, 0x00,	//10: dark green
		0x84, 0x84, 0x00,	//11: dark yellow
		0x00, 0x00, 0x84,	//12: dark blue
		0x84, 0x00, 0x84,	//13: dark magenta
		0x00, 0x84, 0x84,	//14: dark cyan
		0x84, 0x84, 0x84	//15: dark gray
	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb){
	int eflags;
	eflags = io_load_eflags();
	io_cli();
	io_out8(0x03c8, start);
	for(int i=start;i<=end;i++){
		io_out8(0x03c9, rgb[0]/4);
		io_out8(0x03c9, rgb[1]/4);
		io_out8(0x03c9, rgb[2]/4);
		rgb += 3;
	}
	io_store_eflags(eflags);
	return;
}

void init_screen(char *vram, int xsize, int ysize){
	boxfill8(vram, xsize, COL8_008484,	0,			0,			xsize-1,	ysize-29);
	boxfill8(vram, xsize, COL8_C6C6C6,	0,			ysize-28,	xsize-1,	ysize-28);
	boxfill8(vram, xsize, COL8_FFFFFF,	0,			ysize-27,	xsize-1,	ysize-27);
	boxfill8(vram, xsize, COL8_C6C6C6,	0,			ysize-26,	xsize-1,	ysize- 1);

	boxfill8(vram, xsize, COL8_FFFFFF,	 3,			ysize-24,	59,			ysize-24);
	boxfill8(vram, xsize, COL8_FFFFFF,	 2,			ysize-24,	 2,			ysize- 4);
	boxfill8(vram, xsize, COL8_848484,	 3,			ysize- 4,	59,			ysize- 4);
	boxfill8(vram, xsize, COL8_848484,	59,			ysize-23,	59,			ysize- 5);
	boxfill8(vram, xsize, COL8_000000,	 2,			ysize- 3,	59,			ysize- 3);
	boxfill8(vram, xsize, COL8_000000,	60,			ysize-24,	60,			ysize- 3);
	
	boxfill8(vram, xsize, COL8_848484,	xsize-47,	ysize-24,	xsize- 4,	ysize-24);
	boxfill8(vram, xsize, COL8_848484,	xsize-47,	ysize-23,	xsize-47,	ysize- 4);
	boxfill8(vram, xsize, COL8_FFFFFF,	xsize-47,	ysize- 3,	xsize- 4,	ysize- 3);
	boxfill8(vram, xsize, COL8_FFFFFF,	xsize- 3,	ysize-24,	xsize- 3,	ysize- 3);
	
	return;
}

void init_mouse_cursor8(char *mouse, char bc){
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***",
	};
	for(int y=0;y<16;y++){
		for(int x=0;x<16;x++){
			switch(cursor[y][x]){
			case '*':
				mouse[y*16+x] = COL8_000000;
				break;
			case 'O':
				mouse[y*16+x] = COL8_FFFFFF;
				break;
			default:
				mouse[y*16+x] = bc;
				break;
			}
		}
	}
	return;
}

void put_block8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char* buf, int bxsize){
	for(int y=0;y<pysize;y++){
		for(int x=0;x<pxsize;x++){
			vram[(py0+y)*vxsize + (px0+x)] = buf[y * bxsize + x];
		}
	}
	return;
}

void boxfill8(char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1){
	for(int y=y0;y<=y1;y++){
		for(int x=x0;x<=x1;x++){
			vram[y*xsize + x] = c;
		}
	}
	return;
}

void putfont8(char *vram, int xsize, int x, int y, unsigned char c, char *font){
	char *p, d;
	for(int i=0;i<16;i++){
		p = vram + (y+i) * xsize + x;
		d = font[i];
		for(int j=0;j<8;j++){
			if((d & 1<<(7-j)) != 0) *(p+j) = c;
		}
	}
	return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, unsigned char c, unsigned char *s){
	extern char hankaku[4096];
	for(;*s!=0x00;s++){
		putfont8(vram, xsize, x, y, c, hankaku + (*s) * 16);
		x+=8;
	}
	return;
}

void putfonts8_asc_sht(Sheet *sht, int x, int y, int col, int bgcol, char *str, int len){
	boxfill8(sht->buf, sht->bxsize, bgcol, x, y, x + len*8 - 1, y + 15);
	putfonts8_asc(sht->buf, sht->bxsize, x, y, col, str);
	sheet_refresh(sht, x, y, x + len*8, y+16);

	return;
}
