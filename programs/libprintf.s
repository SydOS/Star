global _syscalls_interrupt
_syscalls_interrupt:
    ; Raise syscall interrupt. Passed args should be on stack already.
    ; Return value from handler will be in EAX.
    int 0x80
    ret