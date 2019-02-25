extern int api_openwin(char *, int, int, int, char *);
extern void api_end(void);

char buf[150*50];

void app_main(){
    int win;
    win = api_openwin(buf, 150, 50, -1, "hello");
    api_end();
}
