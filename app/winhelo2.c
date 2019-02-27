#include "api.h"

#define WIN_W 150
#define WIN_H 50

static char buf[WIN_W*WIN_H];

void app_main(){
    int win;
    win = api_openwin(buf, WIN_W, WIN_H, -1, "Hello");
    api_boxfillwin(win, 8, 36, 141, 43, 3);
    api_putstrwin(win, 28, 28, 0, 12, "hello, world");
    api_end();
}