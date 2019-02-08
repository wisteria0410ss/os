#include "bootpack.h"

ShtCtl *shtctl_init(MemMan *memman, unsigned char *vram, int xsize, int ysize){
    ShtCtl *ctl;
    int i;
    ctl = (ShtCtl *) memman_alloc_4k(memman, sizeof(ShtCtl));
    if(ctl == 0) goto err;
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;          // シートなし
    for(i=0;i<MAX_SHEETS;i++) ctl->sheets0[i].flags = 0;    // 未使用
err:
    return ctl;
}

Sheet *sheet_alloc(ShtCtl *ctl){
    Sheet *sht;
    int i;
    for(i=0;i<MAX_SHEETS;i++){
        if(ctl->sheets0[i].flags == 0){
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE;
            sht->height = -1;       // 非表示
            return sht;
        }
    }
    return 0;
}

void sheet_setbuf(Sheet *sht, unsigned char *buf, int xsize, int ysize, int col_transp){
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_transp = col_transp;

    return;
}

void sheet_updown(ShtCtl *ctl, Sheet *sht, int height){
    int h, old = sht->height;

    if(height > ctl->top+1) height = ctl->top + 1;
    if(height < -1) height = -1;
    sht->height = height;

    if(old > height){
        if(height >= 0){
            for(h=old;h>height;h--){
                ctl->sheets[h] = ctl->sheets[h-1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }else{
            if(ctl->top > old){
                for(h=old;h < ctl->top;h++){
                    ctl->sheets[h] = ctl->sheets[h+1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    }else{
        if(old>=0){
            for(h=old;h<height;h++){
                ctl->sheets[h]=ctl->sheets[h+1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }else{
            for(h=ctl->top;h>=height;h--){
                ctl->sheets[h+1] = ctl->sheets[h];
                ctl->sheets[h+1]->height = h+1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    }
    return;
}

void sheet_refresh(ShtCtl *ctl, Sheet *sht, int bx0, int by0, int bx1, int by1){
    if(sht->height >= 0) sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
}

void sheet_refreshsub(ShtCtl *ctl, int vx0, int vy0, int vx1, int vy1){
    int h, bx, by, vx, vy;
    unsigned char *buf, c, *vram = ctl->vram;
    Sheet *sht;

    for(h=0;h<=ctl->top;h++){
        sht = ctl->sheets[h];
        buf = sht->buf;
        for(by=0;by<(sht->bysize);by++){
            vy = sht->vy0 + by;
            for(bx=0;bx<(sht->bxsize);bx++){
                vx = sht->vx0 + bx;
                if(vx0<=vx && vx<vx1 && vy0<=vy && vy<vy1){
                    c = buf[by * sht->bxsize + bx];
                    if(c != sht->col_transp) vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
}

void sheet_slide(ShtCtl *ctl, Sheet *sht, int vx0, int vy0){
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if(sht->height >= 0){
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
    }

    return;
}

void sheet_free(ShtCtl *ctl, Sheet *sht){
    if(sht->height >= 0) sheet_updown(ctl, sht, -1);
    sht->flags = 0;
    return;
}
