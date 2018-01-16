all:
	i686-elf-as boot.asm -o boot.o

	i686-elf-gcc -c main.c -o main.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
	i686-elf-gcc -c vga.c -o vga.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
	i686-elf-gcc -c floppy.c -o floppy.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

	i686-elf-gcc -T linker.ld -o sydos.bin -ffreestanding -O2 -nostdlib *.o -lgcc