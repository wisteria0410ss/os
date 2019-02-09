#include "bootpack.h"

TimerCtl timerctl;

void init_pit(){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    for(int i=0;i<MAX_TIMER;i++) timerctl.timer[i].flags = 0;

    return;
}

Timer *timer_alloc(){
    for(int i=0;i<MAX_TIMER;i++){
        if(timerctl.timer[i].flags == 0){
            timerctl.timer[i].flags == TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return 0;
}

void timer_free(Timer *timer){
    timer->flags = 0;
    return;
}

void timer_init(Timer *timer, FIFO8 *fifo, unsigned char data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(Timer *timer, unsigned int timeout){
    timer->timeout = timeout;
    timer->flags = TIMER_FLAGS_USING;
    return;
}

void inthandler20(int *esp){
    io_out8(PIC0_OCW2, 0x60);       // IRQ-00受付完了をPICに通知
    timerctl.count++;
    for(int i=0;i<MAX_TIMER;i++){
        if(timerctl.timer[i].flags == TIMER_FLAGS_USING){
            timerctl.timer[i].timeout--;
            if(timerctl.timer[i].timeout == 0){
                timerctl.timer[i].flags == TIMER_FLAGS_ALLOC;
                fifo8_push(timerctl.timer[i].fifo, timerctl.timer[i].data);
            }
        }
    }

    return;
}