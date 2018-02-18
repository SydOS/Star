// Defines registers for ISR callbacks.
struct registers
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;    
};
typedef struct registers registers_t;

extern void interrupts_irq_install_handler(uint8_t irq, void (*handler)(registers_t *regs));
extern void interrupts_irq_remove_handler(uint8_t irq);
extern void interrupts_init();
