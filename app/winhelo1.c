#include "api.h"

char buf[150*50];

void app_main(){
    int win;
    win = api_openwin(buf, 150, 50, -1, "hello");
    api_end();
}
