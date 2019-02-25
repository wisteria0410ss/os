#include "bootpack.h"

void console_task(Sheet *sheet, unsigned int memtotal){
	Timer *timer;
	Task *task = task_now();
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	int i, fifobuf[128];
	int *fat = (int *)memman_alloc_4k(memman, 4*2880);
	Console cons;
	char cmdline[128];
	cons.sht = sheet;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	*((int *)0x0fec) = (int)&cons;

	fifo32_init(&task->fifo, 128, fifobuf, task);
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);

	file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));
	
	cons_putchar(&cons, '>', 1);

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
					if(cons.cur_c >= 0) cons.cur_c = COL8_FFFFFF;
				}else{
					timer_init(timer, &task->fifo, 1);
					if(cons.cur_c >= 0) cons.cur_c = COL8_000000;
				}
				timer_settime(timer, 50);
			}
			if(i == 2) cons.cur_c = COL8_FFFFFF;
			if(i == 3){
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				cons.cur_c = -1;
			}
			if(256 <= i && i < 512){
				if(i == 8+256){		// bksp
					if(cons.cur_x > 16){
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				}else if(i == 10 + 256){	// enter
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x/8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);
					
					cons_putchar(&cons, '>', 1);
				}else{
					if(cons.cur_x < CONS_W - 16){
						cmdline[cons.cur_x/8 - 2] = i-256;
						cons_putchar(&cons, i-256, 1);
					}
				}
			}
			if(cons.cur_c >= 0) boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
			sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
		}
	}
}

void cons_putchar(Console *cons, int chr, char move){
	char s[2];
	s[0] = chr;
	s[1] = 0;
	switch (s[0]){
		case 0x09:	// tab
			do{
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
				cons->cur_x += 8;
				if(cons->cur_x >= CONS_W-8) cons_newline(cons);
			}while(((cons->cur_x - 8) & 0x1f) != 0);
			break;
		case 0x0a:	// lf
			cons_newline(cons);
			break;
		case 0x0d:	// cr
			break;
		default:
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
			if(move != 0){
				cons->cur_x += 8;
				if(cons->cur_x >= CONS_W-8) cons_newline(cons);
			}
			break;
	}
	return;
}

void cons_newline(Console *cons){
	Sheet *sheet = cons->sht;
	if(cons->cur_y + 32 < CONS_H - 8) cons->cur_y += 16;
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
	cons->cur_x = 8;
	return;
}

void cons_runcmd(char *cmdline, Console *cons, int *fat, unsigned int memtotal){
	if(strcmp(cmdline, "mem") == 0) cmd_mem(cons, memtotal);
	else if(strcmp(cmdline, "cls") == 0) cmd_cls(cons);
	else if(strcmp(cmdline, "dir") == 0) cmd_dir(cons);
	else if(starts_with(cmdline, "type ") || strcmp(cmdline, "type") == 0) cmd_type(cons, fat, cmdline);
	else if(cmdline[0] != 0){
		if(cmd_app(cons, fat, cmdline) == 0){
			char s[256];
			msprintf(s, "command \'%s\' not found.\n\n", cmdline);
			cons_putstr(cons, s);
		}
	}
	return;
}

void cmd_mem(Console *cons, unsigned int memtotal){
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	char s[60];
	msprintf(s, "Total  %d MiB\nFree   %d kiB\n\n", memtotal / (1024*1024), memman_total(memman) / 1024);
	cons_putstr(cons, s);
	return;
}

void cmd_cls(Console *cons){
	for(int y=28;y<CONS_H-8;y++){
		for(int x=8;x<CONS_W-7;x++){
			cons->sht->buf[x + y*cons->sht->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(cons->sht, 8, 28, CONS_W-7, CONS_H-8);
	cons->cur_y = 28;
	return;
}

void cmd_dir(Console *cons){
	FileInfo *finfo = (FileInfo *)(ADR_DISKIMG + 0x002600);
	char s[30];
	for(int x=0;x<224;x++){
		if(finfo[x].name[0] == 0x00) break;
		if(finfo[x].name[0] != 0xe5){
			if((finfo[x].type & 0x18) == 0){
				msprintf(s, "%.8s.%.3s  %d\n", finfo[x].name, finfo[x].ext, finfo[x].size);
				cons_putstr(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_type(Console *cons, int *fat, char *cmdline){
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	FileInfo *finfo;
	if(cmdline[4] == 0) finfo = 0;
	else finfo = file_search(cmdline+5, (FileInfo *)(ADR_DISKIMG+0x002600), 224);
	
	if(finfo != 0){
		char *p = (char *)memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
		cons_nputstr(cons, p, finfo->size);
		memman_free_4k(memman, (unsigned int)p, finfo->size);
	}else{
		cons_putstr(cons, "File not found.\n");
	}
	cons_newline(cons);
	return;
}

int cmd_app(Console *cons, int *fat, char *cmdline){
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	FileInfo *finfo;
	SegmentDescriptor *gdt = (SegmentDescriptor *)ADR_GDT;
	char name[18], *p;
	int i;

	for(i=0;i<13;i++){
		if(cmdline[i] <= ' ') break;
		name[i] = cmdline[i];
	}
	name[i] = 0;

	finfo = file_search(name, (FileInfo *)(ADR_DISKIMG + 0x002600), 224);
	if(finfo == 0 && name[i-1]!='.'){
		msprintf(name, "%s.HRB", name);
		finfo = file_search(name, (FileInfo *)(ADR_DISKIMG + 0x002600), 224);
	}
	if(finfo != 0){
		p = (char *)memman_alloc_4k(memman, finfo->size);
		*((int *) 0x0fe8) = (int)p;
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
		set_segmdesc(gdt+1003, finfo->size-1, (int)p, AR_CODE32_ER);
		if(finfo->size >= 8 && starts_with(p+4, "Hari")){
			p[0] = 0xe8;
			p[1] = 0x16;
			p[2] = 0x00;
			p[3] = 0x00;
			p[4] = 0x00;
			p[5] = 0xcb;
		}
		farcall(0, 1003*8);
		memman_free_4k(memman, (int)p, finfo->size);
		cons_newline(cons);
		return 1;
	}
	return 0;
}

void cons_putstr(Console *cons, char *s){
	for(;*s!=0;s++) cons_putchar(cons, *s, 1);
	return;
}

void cons_nputstr(Console *cons, char *s, int len){
	for(int i=0;i<len;i++){
		if(s[i] == 0) break;
		cons_putchar(cons, s[i], 1);
	}
	return;
}

void hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax){
	int cs_base = *((int *) 0x0fe8);
	Console *cons = (Console *) *((int *)0x0fec);
	switch(edx){
		case 1:
			cons_putchar(cons, eax & 0xff, 1);
			break;
		case 2:
			cons_putstr(cons, (char *)ebx + cs_base);
			break;
		case 3:
			cons_nputstr(cons, (char *)ebx + cs_base, ecx);
			break;
	}
	return;
}