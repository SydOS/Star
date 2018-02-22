#include <main.h>
#include <kprint.h>
#include <io.h>
#include <arch/i386/kernel/cpuid.h>
#include <arch/i386/kernel/pic.h>

bool lapic_supported() {
    // Check if CPUID is there.
    if (_cpuid_detect()) {
        // Check for the APIC feature.
        uint32_t result, unused;
        cpuid_query(CPUID_GETFEATURES, &unused, &unused, &unused, &result);
        return result & CPUID_FEAT_EDX_APIC;
    }

    return false;
}

void lapic_init() {
    // Disable PIC.
    pic_disable();
    
    


}