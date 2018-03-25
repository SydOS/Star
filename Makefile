CFLAGS?=-std=gnu99 -ffreestanding -ggdb -gdwarf-2 -Wall -Wextra -I./src/include -I./acpica/include
CXXFLAGS?=-std=gnu++14 -ffreestanding -fno-rtti -fno-exceptions -ggdb -gdwarf-2 -Wall -Wextra -I./src/include -I./acpica/include
ARCH?=i686
TIME?=$(shell date +%s)

# Get source files.
ifeq ($(ARCH), x86_64)
IGNOREARCH = i386
else
IGNOREARCH = x86_64
endif
C_SOURCES := $(filter-out $(shell find src/arch/$(IGNOREARCH) -name '*.c'), $(shell find src -name '*.c'))
CPP_SOURCES := $(filter-out $(shell find src/arch/$(IGNOREARCH) -name '*.cpp'), $(shell find src -name '*.cpp'))
ASM_SOURCES := $(filter-out $(shell find src/arch/$(IGNOREARCH) -name '*.asm'), $(shell find src -name '*.asm'))
ACPICA_C_SOURCES := $(shell find acpica -name '*.c')

# Get object files.
C_OBJECTS = $(subst src, build, $(C_SOURCES:.c=.o))
CPP_OBJECTS = $(subst src, build, $(CPP_SOURCES:.cpp=_cpp.o))
ASM_OBJECTS = $(subst src, build, $(ASM_SOURCES:.asm=_asm.o))
ACPICA_C_OBJECTS = $(subst acpica, acpica_build, $(ACPICA_C_SOURCES:.c=.o))

all:
	make clean

	ARCH=i686 make build-kernel

	[[ -d build ]] || mkdir build
	mkdir build/$(TIME)
	cp *.kernel *.sym build/$(TIME)

	make test

build-kernel:
	# Check if we are compiling for different arch, if so clean build dir and save.
ifneq ($(shell cat LASTARCH), $(ARCH))
	rm -rfd acpica_build
	$(shell echo $(ARCH) > LASTARCH)
endif

	# Clean the build directory.
	rm -rfd build

	# Build kernel.
	$(MAKE) build-kernel-main

build-kernel-main: $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) $(ACPICA_C_OBJECTS)
	# Link objects together into binary.
ifeq ($(ARCH), x86_64)
	$(ARCH)-elf-gcc -T src/arch/x86_64/kernel/linker.ld -o Star-$(ARCH).kernel -fPIC -ffreestanding -nostdlib $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) $(ACPICA_C_OBJECTS) -lgcc -z max-page-size=0x1000
else
	$(ARCH)-elf-gcc -T src/arch/i386/kernel/linker.ld -o Star-$(ARCH).kernel -ffreestanding -nostdlib $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) $(ACPICA_C_OBJECTS) -lgcc
endif

	# Strip out debug info into separate files.
	$(ARCH)-elf-objcopy --only-keep-debug Star-$(ARCH).kernel Star-$(ARCH).sym
	$(ARCH)-elf-objcopy --strip-debug Star-$(ARCH).kernel
	# $(ARCH)-elf-objcopy -O binary Star-$(ARCH).kernel Star-$(ARCH).kernel.bin

	# Clean the build directory.
	rm -rfd build

ifeq ($(ARCH), x86_64)
	cp Star-x86_64.kernel isofiles/
	grub-mkrescue -o os.iso isofiles --verbose -d /usr/lib/grub/i386-pc
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

# Compile C++ source files.
$(CPP_OBJECTS):
	mkdir -p $(dir $@)
ifeq ($(ARCH), x86_64)
	$(ARCH)-elf-gcc -c $(subst build, src, $(subst _cpp.o,.cpp,$@)) -o $@ $(CXXFLAGS) -fPIC -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables -g
else
	$(ARCH)-elf-gcc -c $(subst build, src, $(subst _cpp.o,.cpp,$@)) -o $@ $(CXXFLAGS)
endif

# Compile ACPICA C source files.
$(ACPICA_C_OBJECTS):
	mkdir -p $(dir $@)
ifeq ($(ARCH), x86_64)
	$(ARCH)-elf-gcc -c $(subst acpica_build, acpica, $(subst .o,.c,$@)) -o $@ $(CFLAGS) -fPIC -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables -g
else
	$(ARCH)-elf-gcc -c $(subst acpica_build, acpica, $(subst .o,.c,$@)) -o $@ $(CFLAGS)
endif

test:
	qemu-system-x86_64 -kernel Star-i686.kernel -m 32M -d guest_errors -drive format=raw,file=fat12.img,index=0,if=floppy -serial stdio -net nic,model=rtl8139

debug:
	qemu-system-i386 -kernel Star-i686.kernel -S -s & gdb Star-i686.kerel -ex 'target remote localhost:1234'

clean:
	# Clean up binaries.
	rm -rfd build
	rm -rf *.o *.bin *.kernel *.sym *.bin

full-clean:
	# Clean up binaries.
	rm LASTARCH
	rm -rfd build
	rm -rfd acpica_build
	rm -rf *.o *.bin *.kernel *.sym *.bin