#include "bootpack.h"

void fifo8_init(FIFO8 *fifo, int size, unsigned char *buf){
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;      // 空き
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;

    return;
}

int fifo8_push(FIFO8 *fifo, unsigned char data){
    if(fifo->free == 0){    // 溢れた場合
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if(fifo->p == fifo->size) fifo->p = 0;
    fifo->free--;

    return 0;
}

int fifo8_pop(FIFO8 *fifo){
    int data;
    if(fifo->free == fifo->size) return -1;
    data = fifo->buf[fifo->q];
    fifo->q++;
    if(fifo->q == fifo->size) fifo->q = 0;
    fifo->free++;

    return data;
}

int fifo8_status(FIFO8 *fifo){
    return fifo->size - fifo->free;
}

void fifo32_init(FIFO32 *fifo, int size, int *buf, Task *task){
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;      // 空き
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    fifo->task = task;

    return;
}

int fifo32_push(FIFO32 *fifo, int data){
    if(fifo->free == 0){    // 溢れた場合
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if(fifo->p == fifo->size) fifo->p = 0;
    fifo->free--;

    if(fifo->task != 0){
        if(fifo->task->flags != 2) task_run(fifo->task, 0);
    }

    return 0;
}

int fifo32_pop(FIFO32 *fifo){
    int data;
    if(fifo->free == fifo->size) return -1;
    data = fifo->buf[fifo->q];
    fifo->q++;
    if(fifo->q == fifo->size) fifo->q = 0;
    fifo->free++;

    return data;
}

int fifo32_status(FIFO32 *fifo){
    return fifo->size - fifo->free;
}