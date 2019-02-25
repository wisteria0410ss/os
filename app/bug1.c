#include "api.h"

void app_main(){
    char a[100];
    a[10] = 'A';
    api_putchar(a[10]);
    a[102] = 'B';
    api_putchar(a[102]);
    a[12345678] = 'C';
    api_putchar(a[12345678]);
    api_putchar('\n');
    api_end();
}