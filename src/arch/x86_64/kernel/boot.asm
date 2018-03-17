; 32-bit code.
[bits 32]

extern KERNEL_VIRTUAL_OFFSET
extern KERNEL_VIRTUAL_END
extern KERNEL_INIT_END

; Allocate stack
section .bss
    align 16
stack_bottom:
    resb 16384
stack_top:

section .inittables
global DMA_FRAMES_FIRST
DMA_FRAMES_FIRST: resb 4
global DMA_FRAMES_LAST
DMA_FRAMES_LAST: resb 4
global PAGE_FRAME_STACK_START
PAGE_FRAME_STACK_START: resb 4
global PAGE_FRAME_STACK_END
PAGE_FRAME_STACK_END: resb 4
global EARLY_PAGES_LAST
EARLY_PAGES_LAST: resb 4

global MULTIBOOT_MAGIC
MULTIBOOT_MAGIC: resb 4
global MULTIBOOT_INFO
MULTIBOOT_INFO: resb 4

memory: resb 8
memoryEntryEnd: resb 4

; Page table variables.
pagePml4Table: resb 4
pageDirectoryPointerTableLow: resb 4
pageDirectoryLow: resb 4
pageTableLow: resb 4
pageDirectoryKernel: resb 4
pageTableKernelFirst: resb 4
pageTableKernelCurrent: resb 4

; Start function.
section .init
global _start
_start:
    ; Disable interrupts.
    cli
    
    ; Save Multiboot info for later use.
    mov [MULTIBOOT_MAGIC], eax
    mov [MULTIBOOT_INFO], ebx

    ; Ensure CPUID, PAE, and long modes are supported.
    call _check_cpuid
    call _check_pae
    call _check_lm

    ; Get memory map and set up page frame stack.
    call _get_memory_map
    call _setup_stack
    
    ; Set up early 4-level page tables and enable paging.
    call _setup_paging
    call _enable_paging

    ; Load 64-bit GDT.
    lgdt [gdt_descriptor]

    ; Jump to 64-bit higher half.
    jmp 0b1000:_start_higherhalf

; Takes in address in EAX and places a 4KB aligned address in EAX.
_align_4k:
    add eax, 0x1000
    and eax, 0xFFFFF000
    ret

_zero_page:
    ; Target is stored in EAX.
    ; Count is whole 4KB page, stored in ECX. EDX is 0.
    mov edx, 0x1000
    xor ecx, ecx

zero1:
    ; Zero out current position.
    mov [eax], edx
    inc eax

    ; Are we done?
    inc ecx
    cmp ecx, edx
    jb zero1
    ret

_check_cpuid:
    extern _cpuid_detect_early
    call _cpuid_detect_early
    cmp al, 0
    jnz _error

_check_pae:
    ; Ensure features function is supported.
    mov eax, 0x0
    cpuid
    cmp eax, 0x1
    jb _error

    ; Test if the PAE bit is set using the features CPUID function.
    mov eax, 0x1
    cpuid
    test edx, 1 << 6
    jz _error
    ret

_check_lm:
    ; Ensure extended features function is supported.
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb _error

    ; Test if the LM bit is set using the extended features CPUID function.
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz _error
    ret

_get_memory_map:
; Move to first tag in Multiboot header.
    mov eax, [MULTIBOOT_INFO]
    add eax, 8

multiboot_search:
    ; Is the tag a memory map?
    mov edx, [eax]
    cmp edx, 6
    je multiboot_memory_map

    ; Did we reach the end tag?
    cmp edx, 0
    je _error

    ; Add size of entry and move to next tag.
    mov ebx, [eax+4]
    add ebx, 7
    and ebx, ~7
    add eax, ebx

    ; Loop around.
    jmp multiboot_search

    ; Using the memory map from GRUB, count usable memory in system.
    xor edx, edx
    mov [memory], edx

multiboot_memory_map:
    ; Store end of memory map tag.
    mov edx, [eax+4]
    add edx, eax
    mov [memoryEntryEnd], edx

    ; Parse the memory map. We first need to jump to the entries.
    ; Skip over 4 byte size field to entry size and store size in EDX.
    add eax, 8 ; 4 byte type + 4 byte size.
    mov edx, [eax]

    ; Skip over entry version and jump to entries.
    add eax, 8 ; 4 byte entry size + 4 byte entry version.
    jmp multiboot_memory_entry

multiboot_next_mem_entry:
    ; Move to next entry. Entry size is stored in EDX.
    add eax, edx

    ; Is the new address outside the tag size? If so, we are done parsing the memory map.
    mov ebx, [memoryEntryEnd]
    cmp eax, ebx
    jae multiboot_memory_done

multiboot_memory_entry:
    ; Store type in EBX and determine if entry is an available one (0x1).
    ; If not, move to next entry.
    mov ebx, [eax+16]
    cmp ebx, 0x1
    jnz multiboot_next_mem_entry

    ; If we get here, the entry is of type available (0x1).
    ; Get size of entry. Get high and low half and put them into EBX and ECX.
    mov ebx, [eax+8] ; Low 32 bits
    mov ecx, [eax+12] ; High 32 bits

    ; Add to total and move to next entry.
    add [memory], ebx
    adc [memory+4], ecx
    jmp multiboot_next_mem_entry
    
multiboot_memory_done:
    ret

_setup_stack:
    ; Determine start location of DMA frames. This is located after the kernel.
    ; Get first 64KB aligned address after the kernel.
    mov eax, KERNEL_VIRTUAL_END
    add eax, 0x10000
    and eax, 0xFFFF0000
    mov [DMA_FRAMES_FIRST], eax

    ; Determine location of last DMA frame. This is 64 frames, each 64KB each.
    mov eax, [DMA_FRAMES_FIRST]
    add eax, (0x10000 * 64)
    mov [DMA_FRAMES_LAST], eax

    ; Determine start location of page frame stack. This is located after the DMA pages.
    ; Get first 4KB aligned address after the DMA frames.
    mov eax, [DMA_FRAMES_LAST]
    call _align_4k
    mov [PAGE_FRAME_STACK_START], eax

    ; Determine number of page frames.
    ; Get high and low halfs, and store divisor (4KB page frame size) in ECX.
    mov edx, [memory+4]
    mov eax, [memory]
    mov ecx, 0x1000
    div ecx

    ; Determine page frame size and store in EBX.
    mov ebx, 8 ; Space for 64-bit addresses (8 bytes each).
    mul ebx
    mov ebx, eax

    ; Determine end location of page frame stack.
    mov eax, [PAGE_FRAME_STACK_START]
    add eax, ebx ; Add start and size.
    add eax, 8 ; Add extra address.
    mov [PAGE_FRAME_STACK_END], eax
    ret

_setup_paging:
    ;
    ; PML4 TABLE
    ;
    ; Get first 4KB aligned address after the page frame stack.
    mov eax, [PAGE_FRAME_STACK_END]
    sub eax, KERNEL_VIRTUAL_OFFSET
    call _align_4k

    ; Save address of new PML4 table and zero it.
    mov [pagePml4Table], eax
    call _zero_page

    ;
    ; 0GB PDPT
    ;
    ; Get first 4KB aligned address after the PML4 table for 0GB PDPT.
    mov eax, [pagePml4Table]
    call _align_4k

    ; Save address of 0GB page directory pointer table (PDPT) and zero it.
    mov [pageDirectoryPointerTableLow], eax
    call _zero_page
    
    ; Add 0GB PDPT to PML4 table.
    mov eax, [pageDirectoryPointerTableLow]
    mov ebx, [pagePml4Table]
    or eax, 0x3 ; Page is R/W and present.
    mov [ebx], eax

    ;
    ; 0GB PAGE DIRECTORY
    ;
    ; Get first 4KB aligned address after the 0GB PDPT for the 0GB page directory.
    mov eax, [pageDirectoryPointerTableLow]
    call _align_4k
    
    ; Save address of 0GB page directory and zero it.
    mov [pageDirectoryLow], eax
    call _zero_page

    ; Add 0GB page directory to 0GB PDPT.
    mov eax, [pageDirectoryLow]
    mov ebx, [pageDirectoryPointerTableLow]
    or eax, 0x3 ; Page is R/W and present.
    mov [ebx], eax

    ;
    ; LOW MEMORY + .INIT IDENTITY MAPPING TABLE
    ;
    ; Get first 4KB aligned address after the 0GB page directory for the page table used for low memory
    ; and .init section of kernel. This is an identity mapping.
    mov eax, [pageDirectoryLow]
    call _align_4k

    ; Save address of page table and zero it.
    mov [pageTableLow], eax
    call _zero_page

    ; Identity map low memory + ".init" section of kernel.
    ; Store current page in ESI and end page in EDI.
    xor esi, esi
    mov eax, KERNEL_INIT_END
    cdq
    mov ebx, 0x1000
    div ebx
    inc eax
    mov edi, eax

low_loop:
    ; Get current page address and store in EAX.
    mov ebx, 0x1000
    mov eax, esi
    mul ebx

    ; Add flags to page entry and store in ECX
    or eax, 0x3 ; Page is R/W and present.
    mov ecx, eax

    ; Get address of page entry in table and store in EBX.
    mov ebx, [pageTableLow]
    mov eax, 8
    mul esi
    add ebx, eax

    ; Add page to table and move to next page.
    mov [ebx], ecx
    inc esi

    ; Have we hit all the pages?
    cmp esi, edi
    jbe low_loop
    
    ; Add page table to 0GB page directory.
    mov eax, [pageTableLow]
    mov ebx, [pageDirectoryLow]
    or eax, 0x3 ; Page is R/W and present.
    mov [ebx], eax

    ;
    ; KERNEL PAGE DIRECTORY
    ;
    ; Get first 4KB aligned address after the low page table for the page directory used for the kernel.
    mov eax, [pageTableLow]
    call _align_4k

    ; Save address of kernel page directory and zero it.
    mov [pageDirectoryKernel], eax
    call _zero_page

    ; Add kernel page directory to 0GB PDPT.
    mov eax, [pageDirectoryKernel]
    mov ebx, [pageDirectoryPointerTableLow]
    add ebx, (3 * 8) ; 3rd index in PDPT, each entry is 8 bytes.
    or eax, 0x3 ; Page is R/W and present.
    mov [ebx], eax

    ;
    ; KERNEL PAGE TABLES
    ;
    ; Get first 4KB aligned address after the kernel page directory for the first page table used for the kernel.
    mov eax, [pageDirectoryKernel]
    call _align_4k

    ; Save address of kernel page table and zero it.
    mov [pageTableKernelFirst], eax
    mov [pageTableKernelCurrent], eax
    call _zero_page

    ; Add kernel page table to kernel page directory (first index = 0).
    mov eax, [pageTableKernelFirst]
    mov ebx, [pageDirectoryKernel]
    or eax, 0x3 ; Page is R/W and present.
    mov [ebx], eax

    ; Map low memory and kernel to higher-half virtual space.
    ; Store current page in ESI and end page in EDI.
    xor esi, esi ; Start at address/page 0.
    mov eax, [PAGE_FRAME_STACK_END] ; End at page at the end of the page frame stack.
    sub eax, KERNEL_VIRTUAL_OFFSET ; We want the physical address.
    cdq
    mov ebx, 0x1000 ; Each page is 4KB big.
    div ebx
    inc eax
    mov edi, eax ; The number of 4KB pages to map will be stored here.
    
kernel_loop:
    ; Have we reach the need to create another table?
    ; Don't check if we are at page 0.
    cmp esi, 0
    je kernel_map_page

    ; Check if current page is at the 2MB mark (multiple of 512).
    mov eax, esi
    cdq
    mov ebx, 512
    div ebx
    cmp edx, 0
    jnz kernel_map_page       

    ; Get first 4KB aligned address after current kernel page table for the next one.
    mov eax, [pageTableKernelCurrent]
    call _align_4k

    ; Save address of new kernel page table and zero it.
    mov [pageTableKernelCurrent], eax
    call _zero_page

    ; Increase offset in kernel page directory (move to next 8 byte entry).
    mov ecx, [pageDirectoryKernel]
    add ecx, 8
    mov [pageDirectoryKernel], ecx

    ; Add page table to kernel page directory.
    mov eax, [pageTableKernelCurrent]
    mov ebx, [pageDirectoryKernel]
    or eax, 0x3 ; Page is R/W and present.
    mov [ebx], eax

kernel_map_page:
    ; Get current page address and store in EAX.
    mov ebx, 0x1000
    mov eax, esi
    mul ebx

    ; Add flags to page entry and store in ECX
    or eax, 0x3 ; Page is R/W and present.
    mov ecx, eax

    ; Get address of page entry in first table and store in EBX.
    ; The actual address could be in another table, so this requires all
    ; kernel page tables to be consective in memory.
    mov ebx, [pageTableKernelFirst]
    mov eax, 8
    mul esi
    add ebx, eax

    ; Add page to table and move to next page.
    mov [ebx], ecx
    inc esi

    ; Have we hit all the pages?
    cmp esi, edi
    jbe kernel_loop

    ; Recursively map PML4 onto itself using the last entry.
    mov eax, [pagePml4Table]
    mov ebx, eax
    add eax, (511 * 8) ; Get the 511th entry (last).
    or ebx, 0x3
    mov [eax], ebx

    ; Store address of last kernel table page.
    mov eax, [pageTableKernelCurrent]
    mov [EARLY_PAGES_LAST], eax
    ret

_enable_paging:
    ; Enable PAE.
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ; Enable IA-32e mode.
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    ; Enable paging.
    mov eax, [pagePml4Table]
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

_error:
    xchg bx, bx
    hlt

_count_memory:

gdt64:
    dw 0x0000, 0x0000
    db 0x00, 0b00000000, 0b00000000, 0x00

    dw 0xFFFF, 0x0000
    db 0x00, 0b10011010, 0b10101111, 0x00

    dw 0xFFFF, 0x0000
    db 0x00, 0b10010010, 0b11001111, 0x00

    dw 0xFFFF, 0x0000
    db 0x00, 0b11111110, 0b10101111, 0x00

    dw 0xFFFF, 0x0000
    db 0x00, 0b11110010, 0b11001111, 0x00

    ;dw 0x67, 0x0000
    ;db 0x08, 0b11101001, 0b00000000, 0x00
    ;dq 0x0000008000000000>>32
    ;dw 0x00, 0x00

gdt_descriptor:
    dw (5 * 8) - 1
    dq gdt64
    dq 0

; Higher-half of our start function.
section .text
[bits 64]
_start_higherhalf:
    ; Point stack pointer to top.
    mov esp, stack_top

    ; load 0 into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
 
    ; Ensure stack is 16-bit aligned.
    ;and esp, -16

    ; Call the kernel main function!
    extern kernel_main
    call kernel_main
 
    ; Disable interrupts and loop forever.
    cli
.hang:	hlt
    jmp .hang
