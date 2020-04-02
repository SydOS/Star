extern void syscalls_kprintf(const char *format, ...);

__attribute__ ((section (".text.A"))) void main(void) {
    syscalls_kprintf("Hello, World!");
}