extern void api_putstr(char *);
extern void api_end(void);

void app_main(){
    api_putstr("Hello, world!\n");
    api_end();
}