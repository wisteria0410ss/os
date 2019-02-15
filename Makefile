default:
	make img

ipl.bin: ipl.asm Makefile
	nasm ipl.asm -o ipl.bin -l ipl.lst

asmhead.bin: asmhead.asm Makefile
	nasm asmhead.asm -o asmhead.bin -l asmhead.lst

func.o: func.asm Makefile
	nasm -felf func.asm -o func.o -l func.lst

makefont.out: makefont.c
	gcc makefont.c -o makefont.out

hankaku.c: makefont.out hankaku.txt
	./makefont.out > hankaku.c

bootpack.hrb: bootpack.c bootpack.h graphic.c dsctbl.c int.c keyboard.c mouse.c fifo.c memory.c sheet.c timer.c mtask.c har.lds sprintf.c hankaku.c func.o Makefile
	gcc -fno-pie -march=i486 -m32 -masm=intel -nostdlib -T har.lds bootpack.c graphic.c dsctbl.c int.c keyboard.c mouse.c fifo.c memory.c sheet.c timer.c mtask.c sprintf.c hankaku.c func.o -o bootpack.hrb
	
haribote.sys: asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img: ipl.bin haribote.sys Makefile
	mformat -f 1440 -B ipl.bin -C -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::

img: haribote.img

run-vbox: haribote.img
	virtualbox --startvm "Haribote OS"
	
run: haribote.img
	qemu-system-i386 -m 32 -drive file=haribote.img,format=raw,if=floppy -enable-kvm

run-noframe: haribote.img
	qemu-system-i386 -m 32 -no-frame -drive file=haribote.img,format=raw,if=floppy -enable-kvm

clean:
	rm *.bin *.sys *.lst *.img *.hrb *.o *.out hankaku.c