CFLAGS?=-std=gnu99 -O3 -ffreestanding -fno-omit-frame-pointer -ggdb -gdwarf-2 -Wall -Wextra -I./src/include
ARCH?=i686
TIME?=$(shell date +%s)

# Get source files.
C_SOURCES = $(shell find src -name '*.c')
ASM_SOURCES = $(shell find src -iname '*.asm')

# Get object files.
C_OBJECTS = $(subst src, build, $(C_SOURCES:.c=.o))
ASM_OBJECTS = $(subst src, build, $(ASM_SOURCES:.asm=_asm.o))

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

build-kernel: $(ASM_OBJECTS) $(C_OBJECTS)
	# Link objects together into binary.
	$(ARCH)-elf-gcc -T linker.ld -o Star-$(ARCH).kernel -ffreestanding -O2 -nostdlib $(ASM_OBJECTS) $(C_OBJECTS) -lgcc

	# Strip out debug info into separate files.
	$(ARCH)-elf-objcopy --only-keep-debug Star-$(ARCH).kernel Star-$(ARCH).sym
	$(ARCH)-elf-objcopy --strip-debug Star-$(ARCH).kernel
	$(ARCH)-elf-objcopy -O binary Star-$(ARCH).kernel Star-$(ARCH).kernel.bin

	# Clean the build directory.
	rm -rfd build

# Compile assembly source files.
$(ASM_OBJECTS):
	mkdir -p $(dir $@)
ifeq ($(ARCH), x86_64)
	nasm -felf64 $(subst build, src, $(subst _asm.o,.asm,$@)) -o $@
else
	nasm -felf32 $(subst build, src, $(subst _asm.o,.asm,$@)) -o $@
endif

# Compile C source files.
$(C_OBJECTS):
	mkdir -p $(dir $@)
	$(ARCH)-elf-gcc -c $(subst build, src, $(subst .o,.c,$@)) -o $@ $(CFLAGS)

test:
	qemu-system-x86_64 -kernel Star-i686.kernel -m 32M -d guest_errors -drive format=raw,file=fat12.img,index=0,if=floppy -serial stdio

debug:
	qemu-system-i386 -kernel Star-i686.kernel -S -s & gdb Star-i686.kerel -ex 'target remote localhost:1234'

clean:
	# Clean up binaries.
	rm -rfd build
	rm -rf *.o *.bin *.kernel *.sym *.bin