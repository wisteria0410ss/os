#include "bootpack.h"

void console_task(Sheet *sheet, unsigned int memtotal){
	Timer *timer;
	Task *task = task_now();

	int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
	char s[30], cmdline[30], *p;
	MemMan *memman = (MemMan *)MEMMAN_ADDR;
	FileInfo *finfo = (FileInfo *)(ADR_DISKIMG + 0x002600);
	int *fat = (int *)memman_alloc_4k(memman, 4*2880);
	SegmentDescriptor *gdt = (SegmentDescriptor *)ADR_GDT;

	fifo32_init(&task->fifo, 128, fifobuf, task);
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);

	file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));
	
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
					}else if(starts_with(cmdline, "type")){
						for(int y=0;y<11;y++) s[y] = ' ';
						for(int x=5, y=0;y<11 && cmdline[x]!=0;x++){
							if(cmdline[x]=='.' && y<=8) y=8;
							else{
								s[y] = cmdline[x];
								if('a' <= s[y] && s[y] <= 'z') s[y] -= 0x20;
								y++;
							}
						}
						int x;
						for(x=0;x<224;){
							if(finfo[x].name[0] == 0x00) break;
							if((finfo[x].type & 0x18) == 0){
								for(int y=0;y<11;y++){
									if(finfo[x].name[y] != s[y]) goto type_next_file;
								}
								break;
							}
						type_next_file:
							x++;
						}
						if(x<224 && finfo[x].name[0] != 0x00 && cmdline[4] == ' '){
							p = (char *)memman_alloc_4k(memman, finfo[x].size);
							file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
							cursor_x = 8;
							for(int y=0;y<finfo[x].size;y++){
								s[0] = p[y];
								s[1] = 0;
								if(s[0] == 0x09){
									// tab
									do{
										putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
										cursor_x += 8;
										if(cursor_x >= CONS_W-8){
											cursor_x = 8;
											cursor_y = cons_newline(cursor_y, sheet);
										}
									}while(((cursor_x-8) & 0x1f) != 0);
								}else if(s[0] == 0x0a){
									// lf
									cursor_x = 8;
									cursor_y = cons_newline(cursor_y, sheet);
								}else if(s[0] == 0x0d){
									// cr
								}else{
									putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
									cursor_x += 8;
									if(cursor_x >= CONS_W-8){
										cursor_x = 8;
										cursor_y = cons_newline(cursor_y, sheet);
									}
								}
							}
							memman_free_4k(memman, (unsigned int)p, finfo[x].size);
						}else{
							putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
							cursor_y = cons_newline(cursor_y, sheet);
						}
						cursor_y = cons_newline(cursor_y, sheet);
					}else if(strcmp(cmdline, "hlt") == 0){
						int x;
						msprintf(s, "HLT     HRB");
						for(x=0;x<244;){
							if(finfo[x].name[0] == 0x00) break;
							if((finfo[x].type & 0x18) == 0){
								for(int y=0;y<11;y++){
									if(finfo[x].name[y] != s[y]) goto hlt_next_file;
								}
								break;
							}
						hlt_next_file:
							x++;
						}
						if(x<244 && finfo[x].name[0] != 0x00){
							p = (char *)memman_alloc_4k(memman, finfo[x].size);
							file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
							set_segmdesc(gdt + 1003, finfo[x].size-1, (int)p, AR_CODE32_ER);
							farjmp(0, 1003*8);
							memman_free_4k(memman, (int)p, finfo[x].size);
						}else{
							putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
							cursor_y = cons_newline(cursor_y, sheet);
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
