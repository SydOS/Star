CFLAGS?=-std=gnu99 -ffreestanding -O2 -Wall -Wextra -I./include

all:
	i686-elf-as boot.asm -o boot.o

	i686-elf-gcc -c main.c -o main.o $(CFLAGS)
	i686-elf-gcc -c vga.c -o vga.o $(CFLAGS)
	i686-elf-gcc -c floppy.c -o floppy.o $(CFLAGS)
	i686-elf-gcc -c pic.c -o pic.o $(CFLAGS)
	i686-elf-gcc -c driver/nmi.c -o nmi.o $(CFLAGS)

	i686-elf-gcc -T linker.ld -o sydos.bin -ffreestanding -O2 -nostdlib *.o -lgcc

	rm -rf *.o

clean:
	rm -rf *.o *.bin