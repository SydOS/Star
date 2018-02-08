CFLAGS?=-std=gnu99 -ffreestanding -O3 -Wall -Wextra -I./include

all:
	doxygen Doxyfile

	nasm -felf32 src/boot.asm -o boot.o
	nasm -felf32 driver/a20/check_a20.asm -o check_a20.o
	nasm -felf32 driver/a20/enable_a20.asm -o enable_a20.o
	nasm -felf32 driver/pic/disable.asm -o pic_disable.o
	i686-elf-as driver/gdt/gdt.asm -o gdt_asm.o
	i686-elf-as driver/idt/idt.asm -o idt_asm.o

	i686-elf-gcc -c src/main.c -o main.o $(CFLAGS)
	i686-elf-gcc -c src/tools.c -o tools.o $(CFLAGS)
	i686-elf-gcc -c src/logging.c -o logging.o $(CFLAGS)
	i686-elf-gcc -c driver/vga.c -o vga.o $(CFLAGS)
	i686-elf-gcc -c driver/floppy.c -o floppy.o $(CFLAGS)
	i686-elf-gcc -c driver/pic/pic.c -o pic.o $(CFLAGS)
	i686-elf-gcc -c driver/gdt/gdt.c -o gdt.o $(CFLAGS)
	i686-elf-gcc -c driver/idt/idt.c -o idt.o $(CFLAGS)
	i686-elf-gcc -c driver/pit.c -o pit.o $(CFLAGS)
	i686-elf-gcc -c driver/nmi.c -o nmi.o $(CFLAGS)
	i686-elf-gcc -c driver/memory.c -o memory.o $(CFLAGS)
	i686-elf-gcc -c driver/serial.c -o serial.o $(CFLAGS)
	i686-elf-gcc -c driver/paging.c -o paging.o $(CFLAGS)

	i686-elf-gcc -T linker.ld -o Star.kernel -ffreestanding -O2 -nostdlib *.o -lgcc
	i686-elf-objcopy --only-keep-debug Star.kernel Star.sym

	rm -rf *.o

	qemu-system-x86_64 -kernel Star.kernel -m 32M -d guest_errors -fda DISK1.IMA -serial file:serial.log


clean:
	rm -rf *.o *.bin