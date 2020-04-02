.PHONY: helloworld

helloworld:
	nasm -felf32 -F dwarf libprintf.s -o libprintf-s.o
	i686-elf-gcc -c helloworld.c -o helloworld.o -std=gnu99 -ffreestanding -ggdb -gdwarf-2 -Wall -Wextra -Wno-unused-parameter
	i686-elf-gcc -c libprintf.c -o libprintf-c.o -std=gnu99 -ffreestanding -ggdb -gdwarf-2 -Wall -Wextra -Wno-unused-parameter
	
	i686-elf-gcc helloworld.o libprintf-c.o libprintf-s.o -o helloworld.elf32 -ffreestanding -nostdlib -ffunction-sections
	i686-elf-objcopy -O binary helloworld.elf32 helloworld