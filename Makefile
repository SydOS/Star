CFLAGS?=-std=gnu99 -ffreestanding -O3 -Wall -Wextra -I./include

all:
	nasm -felf32 src/boot.asm -o boot.o
	nasm -felf32 driver/a20/check_a20.asm -o check_a20.o
	nasm -felf32 driver/a20/enable_a20.asm -o enable_a20.o
	nasm -felf32 driver/pic/disable.asm -o pic_disable.o

	i686-elf-gcc -c src/main.c -o main.o $(CFLAGS)
	i686-elf-gcc -c driver/vga.c -o vga.o $(CFLAGS)
	i686-elf-gcc -c driver/floppy.c -o floppy.o $(CFLAGS)
	i686-elf-gcc -c driver/pic/pic.c -o pic.o $(CFLAGS)
	i686-elf-gcc -c driver/nmi.c -o nmi.o $(CFLAGS)

	i686-elf-gcc -T linker.ld -o sydos.bin -ffreestanding -O2 -nostdlib *.o -lgcc

	rm -rf *.o

	qemu-system-x86_64 -kernel sydos.bin -fda DISK1.IMA

clean:
	rm -rf *.o *.bin