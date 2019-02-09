#include "bootpack.h"

void init_pit(){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);

    return;
}

void inthandler20(int *esp){
    io_out8(PIC0_OCW2, 0x60);       // IRQ-00受付完了をPICに通知

    return;
}