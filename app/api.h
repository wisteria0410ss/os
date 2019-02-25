// api.asm
extern void api_putchar(int c);
extern void api_putstr(char *str);
extern void api_end(void);
extern int api_openwin(char *buf, int xsize, int ysize, int col_transp, char *title);
extern void api_putstrwin(int win, int x, int y, int col, int len, char *str);
extern void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);