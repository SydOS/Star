CFLAGS?=-std=gnu99 -ffreestanding -O3 -Wall -Wextra -I./include
ARCH?=i686
TIME?=$(shell date +%s)

all:
	make clean

	ARCH=i386 make build-kernel
	ARCH=i486 make build-kernel
	ARCH=i586 make build-kernel
	ARCH=i686 make build-kernel

	[[ -d build ]] || mkdir build
	mkdir build/$(TIME)
	cp *.kernel *.sym *.bin build/$(TIME)

	make test

build-kernel:
	nasm -felf32 src/boot.asm -o boot.o
	nasm -felf32 kernel/a20/check_a20.asm -o check_a20.o
	nasm -felf32 kernel/a20/enable_a20.asm -o enable_a20.o
	nasm -felf32 kernel/cpuid/cpuid.asm -o cpuid_asm.o
	nasm -felf32 kernel/gdt/gdt.asm -o gdt_asm.o
	nasm -felf32 kernel/idt/idt.asm -o idt_asm.o
	nasm -felf32 kernel/interrupts/interrupts.asm -o interrupts_asm.o

	$(ARCH)-elf-gcc -c src/main.c -o main.o $(CFLAGS)
	$(ARCH)-elf-gcc -c src/tools.c -o tools.o $(CFLAGS)
	$(ARCH)-elf-gcc -c src/logging.c -o logging.o $(CFLAGS)

	$(ARCH)-elf-gcc -c kernel/gdt/gdt.c -o gdt.o $(CFLAGS)
	$(ARCH)-elf-gcc -c kernel/idt/idt.c -o idt.o $(CFLAGS)
	$(ARCH)-elf-gcc -c kernel/interrupts/interrupts.c -o interrupts.o $(CFLAGS)
	$(ARCH)-elf-gcc -c kernel/cpuid/cpuid.c -o cpuid.o $(CFLAGS)
	$(ARCH)-elf-gcc -c kernel/pit.c -o pit.o $(CFLAGS)
	$(ARCH)-elf-gcc -c kernel/nmi.c -o nmi.o $(CFLAGS)
	$(ARCH)-elf-gcc -c kernel/memory.c -o memory.o $(CFLAGS)	
	$(ARCH)-elf-gcc -c kernel/paging.c -o paging.o $(CFLAGS)
	
	$(ARCH)-elf-gcc -c driver/serial.c -o serial.o $(CFLAGS)
	$(ARCH)-elf-gcc -c driver/parallel.c -o parallel.o $(CFLAGS)
	$(ARCH)-elf-gcc -c driver/vga.c -o vga.o $(CFLAGS)
	$(ARCH)-elf-gcc -c driver/floppy.c -o floppy.o $(CFLAGS)
	$(ARCH)-elf-gcc -c driver/speaker.c -o speaker.o $(CFLAGS)


	$(ARCH)-elf-gcc -T linker.ld -o Star-$(ARCH).kernel -ffreestanding -O2 -nostdlib *.o -lgcc
	$(ARCH)-elf-objcopy --only-keep-debug Star-$(ARCH).kernel Star-$(ARCH).sym
	$(ARCH)-elf-objcopy --strip-debug Star-$(ARCH).kernel
	$(ARCH)-elf-objcopy -O binary Star-$(ARCH).kernel Star-$(ARCH).kernel.bin

	rm -rf *.o

test:
	qemu-system-x86_64 -kernel Star-i686.kernel -m 32M -d guest_errors -fda DISK1.IMA

debug:
	qemu-system-i386 -kernel Star-i686.kernel -S -s & gdb Star-i686.kerel -ex 'target remote localhost:1234'

clean:
	rm -rf *.o *.bin *.kernel *.sym *.bin