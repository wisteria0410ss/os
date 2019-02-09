#include "bootpack.h"

TimerCtl timerctl;

void init_pit(){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    timerctl.num_using = 0;
    for(int i=0;i<MAX_TIMER;i++) timerctl.timers0[i].flags = 0;

    return;
}

Timer *timer_alloc(){
    for(int i=0;i<MAX_TIMER;i++){
        if(timerctl.timers0[i].flags == 0){
            timerctl.timers0[i].flags == TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
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
    int e, i, j;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;

    e = io_load_eflags();
    io_cli();

    for(i=0;i<timerctl.num_using;i++){
        if(timerctl.timers[i]->timeout >= timer->timeout) break;
    }
    for(j = timerctl.num_using;j>i;j--) timerctl.timers[j] = timerctl.timers[j-1];
    timerctl.num_using++;
    
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(e);
    
    return;
}

void inthandler20(int *esp){
    int i, j;
    io_out8(PIC0_OCW2, 0x60);       // IRQ-00受付完了をPICに通知
    timerctl.count++;
    if(timerctl.count < timerctl.next) return;
    for(i=0;i<timerctl.num_using;i++){
        if(timerctl.timers[i]->timeout > timerctl.count) break;
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo8_push(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }

    timerctl.num_using -= i;
    for(j=0;j<timerctl.num_using;j++) timerctl.timers[j] = timerctl.timers[i+j];

    if(timerctl.num_using > 0) timerctl.next = timerctl.timers[0]->timeout;
    else timerctl.next = 0xffffffff;

    return;
}