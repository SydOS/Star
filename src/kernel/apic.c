#include <main.h>
#include <kprint.h>
#include <io.h>
#include <kernel/pic.h>

void apic_init()
{
    // Disable PIC.
    pic_disable();

    
}