#include "bootpack.h"

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

void memman_init(MemMan *man){
	man->frees = 0;
	man->maxfrees = 0;
	man->lostsize = 0;
	man->losts = 0;
	return;
}

unsigned int memman_total(MemMan *man){
	unsigned int i, t=0;
	for(i=0; i<(man->frees); i++) t += man->free[i].size;

	return t;
}

unsigned int memman_alloc(MemMan *man, unsigned int size){
	unsigned int i, a;
	for(int i=0;i<(man->frees);i++){
		if(man->free[i].size >= size){		// 十分な空きあり
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if(man->free[i].size == 0){
				man->frees--;
				for(;i<man->frees;i++) man->free[i] = man->free[i+1];
			}
			return a;
		}
	}
	return 0;
}

unsigned int memman_alloc_4k(MemMan *man, unsigned int size){
    unsigned int a;
    size = (size + 0xfff) & 0xfffff000;
    a = memman_alloc(man, size);
    return a;
}

int memman_free(MemMan *man, unsigned int addr, unsigned int size){
	int i, j;
	for(i=0;i<(man->frees);i++){
		if(man->free[i].addr > addr) break;
	}

	if(i>0){
		if(man->free[i-1].addr + man->free[i-1].size == addr){		// 前の領域にまとめられる
			man->free[i-1].size += size;
			if(i<(man->frees)){
				if(addr+size == man->free[i].addr){					// 後ろもまとめる
					man->free[i-1].size += man->free[i].size;
					man->frees--;
					for(;i<(man->frees);i++) man->free[i] = man->free[i+1];
				}
			}
			return 0;
		}
	}
	// 前はまとめられなかった
	if(i<(man->frees)){
		if(addr+size == man->free[i].addr){		// 後ろはまとめられる
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}
	// まとめられなかった
	if(man->frees < MEMMAN_FREES){
		for(j=man->frees;j>i;j--) man->free[j] = man->free[j-1];
		man->frees++;
		if(man->maxfrees < man->frees) man->maxfrees = man->frees;
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}
	man->losts++;
	man->lostsize += size;
	return -1;
}

int memman_free_4k(MemMan *man, unsigned int addr, unsigned int size){
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}