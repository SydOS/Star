#include <main.h>
#include <logging.h>
#include <io.h>
#include <kernel/pic.h>

void apic_init()
{
    // Disable PIC.
    pic_disable();

    
}