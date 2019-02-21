#include "bootpack.h"

TimerCtl timerctl;

void init_pit(){
    int i;
    Timer *t;

    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    for(int i=0;i<MAX_TIMER;i++) timerctl.timers0[i].flags = 0;
    t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next_timer = 0;
    timerctl.t0 = t;
    timerctl.next = 0xffffffff;

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

void timer_init(Timer *timer, FIFO32 *fifo, int data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(Timer *timer, unsigned int timeout){
    int e;
    Timer *t, *prev;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;

    e = io_load_eflags();
    io_cli();
    
    t = timerctl.t0;
    if(timer->timeout <= t->timeout){   // 先頭に入れる場合
        timerctl.t0 = timer;
        timer->next_timer = t;
        timerctl.next = timer->timeout;
        io_store_eflags(e);

        return;
    }
    while(1){
        prev = t;
        t = t->next_timer;
        if(timer->timeout <= t->timeout){   // prev と t の間に挿入
            prev->next_timer = timer;
            timer->next_timer = t;
            io_store_eflags(e);

            return;
        }
    }
}

void inthandler20(int *esp){
    char ts = 0;
    Timer *timer;
    io_out8(PIC0_OCW2, 0x60);       // IRQ-00受付完了をPICに通知
    timerctl.count++;
    if(timerctl.count < timerctl.next) return;
    timer = timerctl.t0;
    while(1){
        if(timer->timeout > timerctl.count) break;
        timer->flags = TIMER_FLAGS_ALLOC;
        if(timer != task_timer) fifo32_push(timer->fifo, timer->data);
        else ts = 1;
        timer = timer->next_timer;
    }

    timerctl.t0 = timer;
    timerctl.next = timerctl.t0->timeout;
    
    if(ts != 0) task_switch();
    
    return;
}