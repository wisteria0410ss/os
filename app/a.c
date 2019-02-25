/*void api_putchar(char c){
    __asm__(
        "mov edx, 1\n"
        "mov al, %0\n"
        "int 0x40\n"
    :
    :"r"(c)
    :"edx", "eax");
    return;
}*/
extern void api_putchar(int c);

void app_main(){
    api_putchar('A');
    return;
}
