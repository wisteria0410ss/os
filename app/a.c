/*void api_putchar(char c){
    __asm__(
        "mov edx, 1\n"
        "mov al, %0\n"
        "int 0x40\n"
    :
    :"r"(c)
    :"edx", "eax");
    return;
}
void api_end(){
    __asm(
        "mov    edx, 4\n"
        "int    0x40\n"
    :
    :
    :"edx"
    );
}*/

extern void api_putchar(int c);
extern void api_end(void);

void app_main(){
    api_putchar('A');
    api_end();
}
