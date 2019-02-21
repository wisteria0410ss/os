.PHONY: img run run-noframe run-vbox clean

OBJS := $(patsubst %.c,%.o,$(filter-out makefont.c hankaku.c test.c,$(wildcard *.c))) hankaku.o func.o
FILES := strcmp.c ipl.asm fifo.c

default:
	make img

%.bin: %.asm Makefile
	nasm $< -o $@ -l $(*F).lst

makefont.out: makefont.c
	gcc makefont.c -o makefont.out

hankaku.c: makefont.out hankaku.txt
	./makefont.out > hankaku.c

func.o: func.asm Makefile
	nasm -felf func.asm -o func.o -l func.lst

%.o: %.c bootpack.h Makefile
	gcc -fno-pie -march=i486 -m32 -masm=intel -nostdlib -c $< -o $@

bootpack.hrb: $(OBJS) har.lds Makefile
	gcc -march=i486 -m32 -nostdlib -T har.lds $(OBJS) -o bootpack.hrb
	
haribote.sys: asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img: ipl.bin haribote.sys $(FILES) Makefile
	mformat -f 1440 -B ipl.bin -C -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::
	mcopy $(FILES) -i haribote.img ::

img: haribote.img

run-vbox: haribote.img
	virtualbox --startvm "Haribote OS"
	
run: haribote.img
	qemu-system-i386 -m 32 -drive file=haribote.img,format=raw,if=floppy -enable-kvm

run-noframe: haribote.img
	qemu-system-i386 -m 32 -no-frame -drive file=haribote.img,format=raw,if=floppy -enable-kvm

clean:
	rm *.bin *.sys *.lst *.img *.hrb *.o *.out hankaku.c