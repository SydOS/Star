# Welcome!

This is my toy kernel/operating system project. Not much to look at right now,
but I'm slowly getting places. Check back later if you're interested for some
interesting demos.

### What does it do?

```
- Serial output
- 80x25 VGA output
- GDT
- IDT
- Generic exceptions
- MNI enabling/disabling
- Protected mode
- PIC remapping
- A very very buggy and crashy PIT driver
- Paging
- Input to screen via serial
```

### Building

First you need to build the toolchain needed:
```
cd ./scripts/
./toolchain-i686.sh
[package manager of your choice] install nasm qemu
cd ..
```

Then to build the kernel:
```
make
```