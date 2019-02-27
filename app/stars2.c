#include "api.h"
#include "mt.h"

void app_main(){
    init_genrand(1234);

    char *buf;
    int win, i, x, y;

    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "stars2");
    api_boxfillwin(win + 1,  6, 26, 143, 93, 0);
    for(i=0;i<50;i++){
        x = (genrand_int32() % 137) + 6;
        y = (genrand_int32() %  67) + 26;
        api_point(win + 1, x, y, 3);
    }
    api_refreshwin(win, 6, 26, 144, 94);
    api_end();
}