#include "bootpack.h"

TaskCtl *taskctl;
Timer *task_timer;

Task *task_init(MemMan *memman){
    Task *task, *idle;
    SegmentDescriptor *gdt = (SegmentDescriptor *)ADR_GDT;
    taskctl = (TaskCtl *)memman_alloc_4k(memman, sizeof(TaskCtl));
    for(int i=0;i<MAX_TASKS;i++){
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
    }
    for(int i=0;i<MAX_TASKLEVELS;i++){
        taskctl->level[i].runnning = 0;
        taskctl->level[i].now = 0;
    }
    task = task_alloc();
    task->flags = 2;
    task->priority = 2;
    task->level = 0;
    task_add(task);
    task_switchsub();
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);

    idle = task_alloc();
    idle->tss.esp = memman_alloc_4k(memman, 64*1024) + 64*1024;
    idle->tss.eip = (int)&task_idle;
    idle->tss.es  = 1*8;
    idle->tss.cs  = 2*8;
    idle->tss.ss  = 1*8;
    idle->tss.ds  = 1*8;
    idle->tss.fs  = 1*8;
    idle->tss.gs  = 1*8;
    task_run(idle, MAX_TASKLEVELS - 1, 1);
    
    return task;
}

Task *task_alloc(){
    Task *task;
    for(int i=0;i<MAX_TASKS;i++){
        if(taskctl->tasks0[i].flags == 0){
            task = &taskctl->tasks0[i];
            task->flags = 1;
            task->priority = 2;
            task->tss.eflags = 0x00000202;
            task->tss.eax = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            return task;
        }
    }
    return 0;
}

void task_run(Task *task, int level, int priority){
    if(level < 0) level = task->level;
    if(priority > 0) task->priority = priority;
    if(task->flags == 2 && task->level != level) task_remove(task);
    if(task->flags != 2){
        task->level = level;
        task_add(task);
    }
    taskctl->lv_change = 1;

    return;
}

void task_switch(){
    TaskLevel *tl = &taskctl->level[taskctl->now_lv];
    Task *new_task, *now_task = tl->tasks[tl->now];
    tl->now++;
    if(tl->now == tl->runnning) tl->now = 0;
    if(taskctl->lv_change != 0){
        task_switchsub();
        tl = &taskctl->level[taskctl->now_lv];
    }
    new_task = tl->tasks[tl->now];
    timer_settime(task_timer, new_task->priority);
    if(new_task != now_task) farjmp(0, new_task->sel);
    
    return;
}

void task_sleep(Task *task){
    Task *now_task;
    if(task->flags == 2){
        now_task = task_now();
        task_remove(task);
        if(task == now_task){
            task_switchsub();
            now_task = task_now();
            farjmp(0, now_task->sel);
        }
    }
    return;
}

Task *task_now(){
    TaskLevel *tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now];
}

void task_add(Task *task){
    TaskLevel *tl = &taskctl->level[task->level];
    tl->tasks[tl->runnning] = task;
    tl->runnning++;
    task->flags = 2;

    return;
}

void task_remove(Task *task){
    int i;
    TaskLevel *tl = &taskctl->level[task->level];

    for(i=0;i<tl->runnning;i++){
        if(tl->tasks[i] == task) break;
    }

    tl->runnning--;
    if(i < tl->now) tl->now--;
    if(tl->now >= tl->runnning) tl->now = 0;
    task->flags = 1;

    for(;i<tl->runnning;i++) tl->tasks[i] = tl->tasks[i+1];

    return;
}

void task_switchsub(){
    int i;
    for(i=0;i<MAX_TASKLEVELS;i++){
        if(taskctl->level[i].runnning > 0) break;
    }

    taskctl->now_lv = i;
    taskctl->lv_change = 0;
    return;
}

void task_idle(){
    while(1) io_hlt();
}