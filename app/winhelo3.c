#include "api.h"

void app_main(){
    char *buf;
    int win;

    api_initmalloc();
    buf = api_malloc(150 * 50);
    win = api_openwin(buf, 150, 50, -1, "Hello");
    api_boxfillwin(win,  8, 36, 141, 43, 6);
    api_putstrwin(win, 28, 28, 0, 12, "Hello, world");
    api_end();
}