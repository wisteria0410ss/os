void app_main(){
    __asm__ volatile(
        "mov    eax, 1*8\n"
        "mov    ds, ax\n"
        "movb   [0x102600], 0\n"
        "mov    edx, 4\n"
        "int    0x40\n"
        :
        :
        :"eax", "edx"
    );
}