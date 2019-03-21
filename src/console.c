#include "bootpack.h"

void console_task(Sheet *sheet, unsigned int memtotal){
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
	cons.timer = timer_alloc();
	timer_init(cons.timer, &task->fifo, 1);
	timer_settime(cons.timer, 50);

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
					timer_init(cons.timer, &task->fifo, 0);
					if(cons.cur_c >= 0) cons.cur_c = COL8_FFFFFF;
				}else{
					timer_init(cons.timer, &task->fifo, 1);
					if(cons.cur_c >= 0) cons.cur_c = COL8_000000;
				}
				timer_settime(cons.timer, 50);
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
	char name[18], *p, *q;
	int segsiz, datsiz, esp, dathrb;
	Task *task = task_now();
	ShtCtl *shtctl;
	Sheet *sht;
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
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
		if(finfo->size >= 36 && starts_with(p+4, "Hari") && *p == 0x00){
			segsiz = *((int *)(p+0x0000));
			esp    = *((int *)(p+0x000c));
			datsiz = *((int *)(p+0x0010));
			dathrb = *((int *)(p+0x0014));
			q = (char *)memman_alloc_4k(memman, segsiz);
			*((int *)0x0fe8) = (int)q;
			set_segmdesc(gdt+1003, finfo->size-1, (int)p, AR_CODE32_ER + 0x60);
			set_segmdesc(gdt+1004, segsiz - 1,    (int)q, AR_DATA32_RW + 0x60);
			for(i=0;i<datsiz;i++) q[esp+i] = p[dathrb+i];
			start_app(0x1b, 1003*8, esp, 1004*8, &(task->tss.esp0));
			shtctl = (ShtCtl *)*((int *)0x0fe4);
			for(i=0;i<MAX_SHEETS;i++){
				sht = &(shtctl->sheets0[i]);
				if((sht->flags & 0x11) == 0x11 && sht->task == task) sheet_free(sht);
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int)q, segsiz);
		}else{
			cons_putstr(cons, ".hrb file format error.\n");
		}
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

int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax){
	int ds_base = *((int *) 0x0fe8);
	Task *task = task_now();
	Console *cons = (Console *) *((int *)0x0fec);
	ShtCtl *shtctl = (ShtCtl *) *((int *)0x0fe4);
	Sheet *sht;
	int *reg = &eax + 1, i;

	switch(edx){
		case 1:
			cons_putchar(cons, eax & 0xff, 1);
			break;
		case 2:
			cons_putstr(cons, (char *)ebx + ds_base);
			break;
		case 3:
			cons_nputstr(cons, (char *)ebx + ds_base, ecx);
			break;
		case 4:
			return &(task->tss.esp0);
			break;
		case 5:
			sht = sheet_alloc(shtctl);
			sht->task = task;
			sht->flags |= 0x10;
			sheet_setbuf(sht, (char *)ebx + ds_base, esi, edi, eax);
			make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
			sheet_slide(sht, 100, 50);
			sheet_updown(sht, 3);
			reg[7] = (int)sht;
			break;
		case 6:
			sht = (Sheet *)(ebx & 0xfffffffe);
			putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *)ebp + ds_base);
			if((ebx & 1) == 0) sheet_refresh(sht, esi, edi, esi+ecx*8, edi+16);
			break;
		case 7:
			sht = (Sheet *)(ebx & 0xfffffffe);
			boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
			if((ebx & 1) == 0) sheet_refresh(sht, eax, ecx, esi+1, edi+1);
			break;
		case 8:
			memman_init((MemMan *)(ebx + ds_base));
			ecx &= 0xfffffff0;
			memman_free((MemMan *)(ebx + ds_base), eax, ecx);
			break;
		case 9:
			ecx = (ecx + 0x0f) & 0xfffffff0;
			reg[7] = memman_alloc((MemMan *)(ebx + ds_base), ecx);
			break;
		case 10:
			ecx = (ecx + 0x0f) & 0xfffffff0;
			memman_free((MemMan *)(ebx + ds_base), eax, ecx);
			break;
		case 11:
			sht = (Sheet *)(ebx & 0xfffffffe);
			sht->buf[sht->bxsize * edi + esi] = eax;
			if((ebx & 1) == 0) sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
			break;
		case 12:
			sht = (Sheet *) ebx;
			sheet_refresh(sht, eax, ecx, esi, edi);
			break;
		case 13:
			sht = (Sheet *)(ebx & 0xfffffffe);
			hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
			if((ebx & 1) == 0) sheet_refresh(sht, eax, ecx, esi+1, edi+1);
			break;
		case 14:
			sheet_free((Sheet *) ebx);
			break;
		case 15:
			while(1){
				io_cli();
				if(fifo32_status(&task->fifo) == 0){
					if(eax != 0) task_sleep(task);
					else{
						io_sti();
						reg[7] = -1;
						return 0;
					}
				}
				i = fifo32_pop(&task->fifo);
				io_sti();
				if(i <= 1){
					timer_init(cons->timer, &task->fifo, 1);
					timer_settime(cons->timer, 50);
				}
				if(i == 2){
					cons->cur_c = COL8_000000;
				}
				if(i == 3){
					cons->cur_c = -1;
				}
				if(i >= 256){
					reg[7] = i-256;
					return 0;
				}
			}
			break;
		case 16:
			reg[7] = (int)timer_alloc();
			((Timer *)reg[7])->flags2 = 1;
			break;
		case 17:
			timer_init((Timer *)ebx, &task->fifo, eax+256);
			break;
		case 18:
			timer_settime((Timer *)ebx, eax);
			break;
		case 19:
			timer_free((Timer *)ebx);
			break;
	}
	return 0;
}

void hrb_api_linewin(Sheet *sht, int x0, int y0, int x1, int y1, int col){
	int i, x, y, len, dx, dy;

	dx = x1-x0;
	dy = y1-y0;
	x = x0 << 10;
	y = y0 << 10;
	if(dx < 0) dx *= -1;
	if(dy < 0) dy *= -1;
	if(dx >= dy){
		len = dx + 1;
		if(x0 > x1) dx = -1024;
		else dx = 1024;
		if(y0 <= y1) dy = ((y1-y0+1) << 10) / len;
		else dy = ((y1-y0-1) << 10) / len;
	}else{
		len = dy + 1;
		if(y0 > y1) dy = -1024;
		else dy = 1024;
		if(x0 <= x1) dx = ((x1-x0+1) << 10) / len;
		else dx = ((x1-x0-1) << 10) / len;
	}

	for(i=0;i<len;i++){
		sht->buf[(y>>10) * sht->bxsize + (x>>10)] = col;
		x += dx;
		y += dy;
	}

	return;
}

int *inthandler0c(int *esp){
	Console *cons = (Console *) *((int *)0x0fec);
	Task *task = task_now();
	char s[30];
	cons_putstr(cons, "\nINT 0C: Stack Exception.\n");
	msprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr(cons, s);
	return &(task->tss.esp0);
}

int *inthandler0d(int *esp){
	Console *cons = (Console *) *((int *)0x0fec);
	Task *task = task_now();
	char s[30];
	cons_putstr(cons, "\nINT 0D: General Protected Exception.\n");
	msprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr(cons, s);
	return &(task->tss.esp0);
}