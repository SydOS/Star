#include <main.h>
#include <tools.h>
#include <logging.h>
#include <driver/cpuid.h>

extern uint32_t _cpuid_detect();
extern uint32_t _cpuid_gethighestfunction();
extern uint32_t _cpuid_getsignature();
extern uint32_t _cpuid_getmiscinfo();
extern uint32_t _cpuid_getfeatures();
extern uint32_t _cpuid_getfeatures2();
extern uint32_t _cpuid_gethighestextendedfunction();

void cpuid_print_capabilities() {
    char temp1[32];

    // Determine if CPUID is even supported.
    // A return value of 0 indicates CPUID is supported.
    if(_cpuid_detect() != 0) {
        // Get highest CPUID function supported
        uint32_t highest_cpuidfunc = _cpuid_gethighestfunction();

        // Print vendor ID if supported (should be).
	    log("Processor vendor: ");
	    uint32_t cpuvendor[4];
	    cpuid_vendorstring(cpuvendor);
        cpuvendor[3] = '\0';
        log((char*)cpuvendor);
	    log("\n");

        // Print processor info and feature bits if supported.
        if (highest_cpuidfunc >= CPUID_GETFEATURES) {
            // Get the processor signature.
            uint32_t signature = _cpuid_getsignature();
            log("Family ");
            utoa((signature & 0x00000F00) >> 8, temp1, 10);
            log(temp1);
            log(", model ");
            utoa((signature & 0x000000F0) >> 4, temp1, 10);
            log(temp1);
            log(", stepping ");
            utoa(signature & 0x0000000F, temp1, 10);
            log(temp1);
            log("\n");

            // Get processor features.
            log("Supported features: ");

            // EDX features.
            uint32_t features = _cpuid_getfeatures();
            if (features & 1)
                log("FPU "); // Onboard x87 FPU.
            if ((features >> 1) & 1)
                log("VME "); // Virtual 8086 mode extensions.
            if ((features >> 2) & 1)
                log("DE "); // Debugging extensions.
            if ((features >> 3) & 1)
                log("PSE "); // Page Size Extension.
            if ((features >> 4) & 1)
                log("TSC "); // Time Stamp Counter.
            if ((features >> 5) & 1)
                log("MSR "); // Model-specific registers.
            if ((features >> 6) & 1)
                log("PAE "); // Physical Address Extension.
            if ((features >> 7) & 1)
                log("MCE "); // Machine Check Exception.
            if ((features >> 8) & 1)
                log("CX8 "); // CMPXCHG8 (compare-and-swap) instruction.
            if ((features >> 9) & 1)
                log("APIC "); // Onboard Advanced Programmable Interrupt Controller.
            //if ((features >> 10) & 1)
              //  log(" "); // Reserved.
            if ((features >> 11) & 1)
                log("SEP "); // SYSENTER and SYSEXIT instructions.
            if ((features >> 12) & 1)
                log("MTRR "); // Memory Type Range Registers.
            if ((features >> 13) & 1)
                log("PGE "); // Page Global Enable bit in CR4.
            if ((features >> 14) & 1)
                log("MCA "); // Machine check architecture.
            if ((features >> 15) & 1)
                log("CMOV "); // Conditional move and FCMOV instructions.
            if ((features >> 16) & 1)
                log("PAT "); // Page Attribute Table.
            if ((features >> 17) & 1)
                log("PSE-36 "); // 36-bit page size extension.
            if ((features >> 18) & 1)
                log("PSN "); // Processor Serial Number.
            if ((features >> 19) & 1)
                log("CLFSH "); // CLFLUSH instruction.
            //if ((features >> 20) & 1)
            //    log(" "); // Reserved.
            if ((features >> 21) & 1)
                log("DS "); // Debug store: save trace of executed jumps.
            if ((features >> 22) & 1)
                log("ACPI "); // Onboard thermal control MSRs for ACPI.
            if ((features >> 23) & 1)
                log("MMX "); // MMX instructions.
            if ((features >> 24) & 1)
                log("FXSR "); // FXSAVE, FXRESTOR instructions.
            if ((features >> 25) & 1)
                log("SSE "); // SSE instructions.
            if ((features >> 26) & 1)
                log("SSE2 "); // SSE2 instructions.
            if ((features >> 27) & 1)
                log("SS "); // CPU cache supports self-snoop.
            if ((features >> 28) & 1)
                log("HTT "); // Hyper-threading.
            if ((features >> 29) & 1)
                log("TM "); // Thermal monitor automatically limits temperature.
            if ((features >> 30) & 1)
                log("IA64 "); // IA64 processor emulating x86.
            if ((features >> 31) & 1)
                log("PBE "); // Pending Break Enable (PBE# pin) wakeup support.
            
            // ECX features2.
            uint32_t features2 = _cpuid_getfeatures2();
            if (features2 & 1)
                log("SSE3 "); // SSE3 instructions.
            if ((features2 >> 1) & 1)
                log("PCLMULQDQ "); // PCLMULQDQ support.
            if ((features2 >> 2) & 1)
                log("DTES64 "); // 64-bit debug store.
            if ((features2 >> 3) & 1)
                log("MONITOR "); // MONITOR and MWAIT instructions.
            if ((features2 >> 4) & 1)
                log("DS-CPL "); // CPL qualified debug store.
            if ((features2 >> 5) & 1)
                log("VMX "); // Virtual Machine eXtensions.
            if ((features2 >> 6) & 1)
                log("SMX "); // Safer Mode Extensions.
            if ((features2 >> 7) & 1)
                log("EST "); // Enhanced SpeedStep.
            if ((features2 >> 8) & 1)
                log("TM2 "); // Thermal Monitor 2.
            if ((features2 >> 9) & 1)
                log("SSSE3 "); // Supplemental SSE3 instructions.
            if ((features2 >> 10) & 1)
                log("CNXT-ID "); // L1 Context ID.
            if ((features2 >> 11) & 1)
                log("SDBG "); // Silicon Debug interface.
            if ((features2 >> 12) & 1)
                log("FMA "); // Fused multiply-add.
            if ((features2 >> 13) & 1)
                log("CX16 "); // CMPXCHG16B instruction.
            if ((features2 >> 14) & 1)
                log("XTPR "); // Can disable sending task priority messages.
            if ((features2 >> 15) & 1)
                log("PDCM "); // Perfmon & debug capability.
            //if ((features2 >> 16) & 1)
            //    log(" "); // Reserved.
            if ((features2 >> 17) & 1)
                log("PCID "); // Process context identifiers.
            if ((features2 >> 18) & 1)
                log("DCA "); // Direct cache access for DMA writes.
            if ((features2 >> 19) & 1)
                log("SSE4.1 "); // SSE4.1 instructions.
            if ((features2 >> 20) & 1)
                log("SSE4.2 "); // SSE4.2 instructions.
            if ((features2 >> 21) & 1)
                log("X2APIC "); // x2APIC support.
            if ((features2 >> 22) & 1)
                log("MOVBE "); // MOVBE instruction (big-endian).
            if ((features2 >> 23) & 1)
                log("POPCNT "); // POPCNT instruction.
            if ((features2 >> 24) & 1)
                log("TSC-DEADLINE "); // APIC supports one-shot operation using a TSC deadline value.
            if ((features2 >> 25) & 1)
                log("AES "); // AES instruction set.
            if ((features2 >> 26) & 1)
                log("XSAVE "); // XSAVE, XRESTOR, XSETBV, XGETBV.
            if ((features2 >> 27) & 1)
                log("OSXSAVE "); // XSAVE enabled by OS.
            if ((features2 >> 28) & 1)
                log("AVX "); // Advanced Vector Extensions.
            if ((features2 >> 29) & 1)
                log("F16C "); // F16C (half-precision) FP support.
            if ((features2 >> 30) & 1)
                log("RDRND "); // RDRAND (on-chip random number generator) support.
            if ((features2 >> 31) & 1)
                log("HYPERVISOR "); // Running on a hypervisor (always 0 on a real CPU, but also with some hypervisors).
            
            log("\n");
        }

        // Get highest extended CPUID function supported
        uint32_t highestextended_cpuidfunc = _cpuid_gethighestextendedfunction();

        // Does this processor support extended CPUID features?
        if (highestextended_cpuidfunc >= CPUID_INTELFEATURES) {
            uint32_t cpuid[5];
    	    cpuid_string(CPUID_INTELBRANDSTRING, cpuid);
    	    cpuid[4] = '\0';
    	    log((char*)cpuid);

    	    cpuid_string(CPUID_INTELBRANDSTRINGMORE, cpuid);
    	    cpuid[4] = '\0';
    	    log((char*)cpuid);
    
    	    cpuid_string(CPUID_INTELBRANDSTRINGEND, cpuid);
    	    cpuid[4] = '\0';
    	    log((char*)cpuid);
    	    log("\n");
        }
    }
    else {
        // CPUID is not supported.
        log("CPUID is not supported.\n");
    }
}
