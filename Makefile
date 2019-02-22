.PHONY: img run run-noframe run-vbox clean

OBJS := $(patsubst src/%.c,obj/%.o,$(filter-out src/hankaku.c src/test.c,$(wildcard src/*.c))) obj/hankaku.o obj/func.o
FILES := src/strcmp.c src/ipl.asm src/fifo.c bin/hlt.hrb

default:
	make img

bin/%.bin: src/%.asm Makefile
	nasm $< -o $@ -l lst/$(*F).lst

util/makefont.out: util/makefont.c Makefile
	gcc $< -o $@

src/hankaku.c: util/makefont.out src/hankaku.txt Makefile
	util/makefont.out src/hankaku.txt > src/hankaku.c

obj/func.o: src/func.asm Makefile
	nasm -felf $< -o $@ -l lst/$(*F).lst

obj/%.o: src/%.c src/bootpack.h Makefile
	gcc -fno-pie -march=i486 -m32 -masm=intel -nostdlib -c $< -o $@

bin/bootpack.hrb: $(OBJS) src/har.lds Makefile
	gcc -march=i486 -m32 -nostdlib -T src/har.lds $(OBJS) -Xlinker -Map -Xlinker lst/bootpack.map -o $@
	
bin/haribote.sys: bin/asmhead.bin bin/bootpack.hrb Makefile
	cat bin/asmhead.bin bin/bootpack.hrb > $@

bin/hlt.hrb: app/hlt.asm Makefile
	nasm $< -o $@

haribote.img: bin/ipl.bin bin/haribote.sys $(FILES) Makefile
	mformat -f 1440 -B bin/ipl.bin -C -i haribote.img ::
	mcopy bin/haribote.sys $(FILES) -i haribote.img ::

img: haribote.img

run-vbox: haribote.img
	virtualbox --startvm "Haribote OS"
	
run: haribote.img
	qemu-system-i386 -m 32 -drive file=haribote.img,format=raw,if=floppy -enable-kvm

run-noframe: haribote.img
	qemu-system-i386 -m 32 -no-frame -drive file=haribote.img,format=raw,if=floppy -enable-kvm

clean:
	-rm bin/*.bin bin/*.sys lst/*.lst *.img bin/*.hrb obj/*.o util/*.out src/hankaku.c