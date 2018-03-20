CFLAGS?=-std=gnu99 -ffreestanding -ggdb -gdwarf-2 -Wall -Wextra -I./src/include
ARCH?=i686
TIME?=$(shell date +%s)

# Get source files.
ifeq ($(ARCH), x86_64)
IGNOREARCH = i386
else
IGNOREARCH = x86_64
endif
C_SOURCES = $(filter-out $(shell find src/arch/$(IGNOREARCH) -name '*.c'), $(shell find src -name '*.c'))
ASM_SOURCES = $(filter-out $(shell find src/arch/$(IGNOREARCH) -name '*.asm'), $(shell find src -name '*.asm'))

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
	cp *.kernel *.sym build/$(TIME)

	make test

build-kernel: $(ASM_OBJECTS) $(C_OBJECTS)
	@echo $(C_SOURCES)
	@echo $(ARCH)
	@echo $(abspath src/main.c)
	# Link objects together into binary.
ifeq ($(ARCH), x86_64)
	$(ARCH)-elf-gcc -T src/arch/x86_64/kernel/linker.ld -o Star-$(ARCH).kernel -fPIC -ffreestanding -nostdlib $(ASM_OBJECTS) $(C_OBJECTS) -lgcc -z max-page-size=0x1000
else
	$(ARCH)-elf-gcc -T src/arch/i386/kernel/linker.ld -o Star-$(ARCH).kernel -ffreestanding -nostdlib $(ASM_OBJECTS) $(C_OBJECTS) -lgcc
endif

	# Strip out debug info into separate files.
	$(ARCH)-elf-objcopy --only-keep-debug Star-$(ARCH).kernel Star-$(ARCH).sym
	$(ARCH)-elf-objcopy --strip-debug Star-$(ARCH).kernel
	# $(ARCH)-elf-objcopy -O binary Star-$(ARCH).kernel Star-$(ARCH).kernel.bin

	# Clean the build directory.
	rm -rfd build

ifeq ($(ARCH), x86_64)
	#cp Star-x86_64.kernel isofiles/
	#grub-mkrescue -o os.iso isofiles --verbose -d /usr/lib/grub/i386-pc
endif

# Compile assembly source files.
$(ASM_OBJECTS):
	mkdir -p $(dir $@)
ifeq ($(ARCH), x86_64)
	nasm -felf64 -F dwarf $(subst build, src, $(subst _asm.o,.asm,$@)) -o $@
else
	nasm -felf32 -F dwarf $(subst build, src, $(subst _asm.o,.asm,$@)) -o $@
endif

# Compile C source files.
$(C_OBJECTS):
	mkdir -p $(dir $@)
ifeq ($(ARCH), x86_64)
	$(ARCH)-elf-gcc -c $(subst build, src, $(subst .o,.c,$@)) -o $@ $(CFLAGS) -fPIC -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables -g
else
	$(ARCH)-elf-gcc -c $(subst build, src, $(subst .o,.c,$@)) -o $@ $(CFLAGS)
endif

test:
	qemu-system-x86_64 -kernel Star-i686.kernel -m 32M -d guest_errors -drive format=raw,file=fat12.img,index=0,if=floppy -serial stdio -net nic,model=rtl8139

debug:
	qemu-system-i386 -kernel Star-i686.kernel -S -s & gdb Star-i686.kerel -ex 'target remote localhost:1234'

clean:
	# Clean up binaries.
	rm -rfd build
	rm -rf *.o *.bin *.kernel *.sym *.bin