// api.asm
extern void api_putchar(int c);
extern void api_putstr(char *str);
extern void api_end(void);
extern int api_openwin(char *buf, int xsize, int ysize, int col_transp, char *title);
extern void api_putstrwin(int win, int x, int y, int col, int len, char *str);
extern void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
extern void api_initmalloc(void);
extern char *api_malloc(int size);
extern void api_free(char *addr, int size);
extern void api_point(int win, int x, int y, int col);
extern void api_refreshwin(int win, int x0, int y0, int x1, int y1);
extern void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
extern void api_closewin(int win);
extern int api_getkey(int mode);