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
    timerctl.num_using++;
    
    if(timerctl.num_using == 1){        // 動作中のものが1つのみになる場合
        timerctl.t0 = timer;
        timer->next_timer = 0;
        timerctl.next = timer->timeout;
        io_store_eflags(e);

        return;
    }
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
        if(t == 0) break;   // 末尾に到達
        if(timer->timeout <= t->timeout){   // prev と t の間に挿入
            prev->next_timer = timer;
            timer->next_timer = t;
            io_store_eflags(e);

            return;
        }
    }
    // 末尾に挿入
    prev->next_timer = timer;
    timer->next_timer = 0;
    io_store_eflags(e);

    return;
}

void inthandler20(int *esp){
    int i;
    Timer *timer;
    io_out8(PIC0_OCW2, 0x60);       // IRQ-00受付完了をPICに通知
    timerctl.count++;
    if(timerctl.count < timerctl.next) return;
    timer = timerctl.t0;
    for(i=0;i<timerctl.num_using;i++){
        if(timer->timeout > timerctl.count) break;
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_push(timer->fifo, timer->data);
        timer = timer->next_timer;
    }

    timerctl.num_using -= i;
    timerctl.t0 = timer;

    if(timerctl.num_using > 0) timerctl.next = timerctl.t0->timeout;
    else timerctl.next = 0xffffffff;

    return;
}