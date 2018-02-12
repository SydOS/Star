#include <main.h>
#include <logging.h>
#include <kernel/idt.h>
#include <io.h>

extern void _idt_load(uint32_t ptr);

// Create an IDT with 256 entries.
idt_entry_t idt[256];
idt_ptr_t idt_ptr;

// Sets an entry in the IDT.
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    // The base address of the interrupt routine.
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;

    // The segment of the IDT entry.
    idt[num].sel = sel;
    idt[num].always0 = 0;
    // We must uncomment the OR below when we get to using user-mode.
    // It sets the interrupt gate's privilege level to 3.
    idt[num].flags   = flags /* | 0x60 */;
}

// Initializes the IDT.
void idt_init()
{
    // Set up the IDT pointer and limit.
    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base = &idt;

    // Clear out the IDT with zeros.
    memset(&idt, 0, sizeof(idt_entry_t) * 256);

    // Load the IDT into the processor.
    _idt_load((uint32_t)&idt_ptr);
    log("IDT initialized!\n");
}
