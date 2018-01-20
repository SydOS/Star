extern void idt_init();
extern void idt_register_interrupt(uint8_t i, uint32_t callback);