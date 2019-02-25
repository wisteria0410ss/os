.PHONY: img run run-noframe run-vbox clean

OBJS := $(patsubst src/%.c,obj/%.o,$(filter-out src/hankaku.c src/test.c,$(wildcard src/*.c))) obj/hankaku.o obj/func.o
APPS := $(patsubst app/%.asm,bin/%.hrb,$(filter-out app/a_nasm.asm,$(wildcard app/*.asm))) bin/a.hrb bin/hello3.hrb
FILES := src/strcmp.c src/ipl.asm src/fifo.c $(APPS)

default:
	make img

bin/%.bin: src/%.asm Makefile
	nasm $< -o $@ -l lst/$(*F).lst

bin/%.hrb: app/%.asm Makefile
	nasm $< -o $@ -l lst/$(*F).lst

obj/a.o: app/a_nasm.asm Makefile
	nasm -felf app/a_nasm.asm -o obj/a.o

bin/a.hrb: app/a.c obj/a.o app/har.lds Makefile
	gcc -fno-pie -march=i486 -m32 -masm=intel -nostdlib -T app/har.lds app/a.c obj/a.o -o $@

bin/crack.hrb: app/crack.c Makefile
	gcc -fno-pie -march=i486 -m32 -masm=intel -nostdlib -T app/har.lds $< -o $@

bin/hello3.hrb: app/hello3.c obj/a.o app/har.lds Makefile
	gcc -fno-pie -march=i486 -m32 -masm=intel -nostdlib -T app/har.lds app/hello3.c obj/a.o -o $@

util/makefont.out: util/makefont.c Makefile
	gcc $< -o $@

src/hankaku.c: util/makefont.out src/hankaku.txt Makefile
	util/makefont.out src/hankaku.txt > src/hankaku.c

obj/func.o: src/func.asm Makefile
	nasm -felf $< -o $@ -l lst/$(*F).lst

obj/%.o: src/%.c src/bootpack.h Makefile
	gcc -fno-pie -march=i486 -m32 -masm=intel -nostdlib -c $< -o $@

bin/bootpack.hrb: $(OBJS) src/har.lds Makefile
	ld -o $@ $(OBJS) -e os_main -Map lst/bootpack.map -m elf_i386 -T src/har.lds
	#gcc -march=i486 -m32 -nostdlib -T src/har.lds $(OBJS) -Wl,-Map=lst/bootpack.map -o $@

bin/haribote.sys: bin/asmhead.bin bin/bootpack.hrb Makefile
	cat bin/asmhead.bin bin/bootpack.hrb > $@

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
	-rm bin/*.bin bin/*.sys lst/* *.img bin/*.hrb obj/*.o util/*.out src/hankaku.c