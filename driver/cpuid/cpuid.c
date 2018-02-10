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
extern uint32_t _cpuid_getextendedprocessorfeatures();
extern uint32_t _cpuid_getextendedprocessorfeatures2();

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
            log("Features: ");

            // EDX features.
            uint32_t features = _cpuid_getfeatures();
            if (features & CPUID_FEAT_EDX_FPU)
                log("FPU "); // Onboard x87 FPU.
            if (features & CPUID_FEAT_EDX_VME)
                log("VME "); // Virtual 8086 mode extensions.
            if (features & CPUID_FEAT_EDX_DE)
                log("DE "); // Debugging extensions.
            if (features & CPUID_FEAT_EDX_PSE)
                log("PSE "); // Page Size Extension.
            if (features & CPUID_FEAT_EDX_TSC)
                log("TSC "); // Time Stamp Counter.
            if (features & CPUID_FEAT_EDX_MSR)
                log("MSR "); // Model-specific registers.
            if (features & CPUID_FEAT_EDX_PAE)
                log("PAE "); // Physical Address Extension.
            if (features & CPUID_FEAT_EDX_MCE)
                log("MCE "); // Machine Check Exception.
            if (features & CPUID_FEAT_EDX_CX8)
                log("CX8 "); // CMPXCHG8 (compare-and-swap) instruction.
            if (features & CPUID_FEAT_EDX_APIC)
                log("APIC "); // Onboard Advanced Programmable Interrupt Controller.
            if (features & CPUID_FEAT_EDX_SEP)
                log("SEP "); // SYSENTER and SYSEXIT instructions.
            if (features & CPUID_FEAT_EDX_MTRR)
                log("MTRR "); // Memory Type Range Registers.
            if (features & CPUID_FEAT_EDX_PGE)
                log("PGE "); // Page Global Enable bit in CR4.
            if (features & CPUID_FEAT_EDX_MCA)
                log("MCA "); // Machine check architecture.
            if (features & CPUID_FEAT_EDX_CMOV)
                log("CMOV "); // Conditional move and FCMOV instructions.
            if (features & CPUID_FEAT_EDX_PAT)
                log("PAT "); // Page Attribute Table.
            if (features & CPUID_FEAT_EDX_PSE36)
                log("PSE36 "); // 36-bit page size extension.
            if (features & CPUID_FEAT_EDX_PSN)
                log("PSN "); // Processor Serial Number.
            if (features & CPUID_FEAT_EDX_CLF)
                log("CLFSH "); // CLFLUSH instruction.
            if (features & CPUID_FEAT_EDX_DTES)
                log("DS "); // Debug store: save trace of executed jumps.
            if (features & CPUID_FEAT_EDX_ACPI)
                log("ACPI "); // Onboard thermal control MSRs for ACPI.
            if (features & CPUID_FEAT_EDX_MMX)
                log("MMX "); // MMX instructions.
            if (features & CPUID_FEAT_EDX_FXSR)
                log("FXSR "); // FXSAVE, FXRESTOR instructions.
            if (features & CPUID_FEAT_EDX_SSE)
                log("SSE "); // SSE instructions.
            if (features & CPUID_FEAT_EDX_SSE2)
                log("SSE2 "); // SSE2 instructions.
            if (features & CPUID_FEAT_EDX_SS)
                log("SS "); // CPU cache supports self-snoop.
            if (features & CPUID_FEAT_EDX_HTT)
                log("HTT "); // Hyper-threading.
            if (features & CPUID_FEAT_EDX_TM1)
                log("TM "); // Thermal monitor automatically limits temperature.
            if (features & CPUID_FEAT_EDX_IA64)
                log("IA64 "); // IA64 processor emulating x86.
            if (features & CPUID_FEAT_EDX_PBE)
                log("PBE "); // Pending Break Enable (PBE# pin) wakeup support.
            
            // ECX features.     
            uint32_t features2 = _cpuid_getfeatures2();
            if (features2)
                log("\n");
            if (features2 & CPUID_FEAT_ECX_SSE3)
                log("SSE3 "); // SSE3 instructions.
            if (features2 & CPUID_FEAT_ECX_PCLMUL)
                log("PCLMULQDQ "); // PCLMULQDQ support.
            if (features2 & CPUID_FEAT_ECX_DTES64)
                log("DTES64 "); // 64-bit debug store.
            if (features2 & CPUID_FEAT_ECX_MONITOR)
                log("MONITOR "); // MONITOR and MWAIT instructions.
            if (features2 & CPUID_FEAT_ECX_DS_CPL)
                log("DS-CPL "); // CPL qualified debug store.
            if (features2 & CPUID_FEAT_ECX_VMX)
                log("VMX "); // Virtual Machine eXtensions.
            if (features2 & CPUID_FEAT_ECX_SMX)
                log("SMX "); // Safer Mode Extensions.
            if (features2 & CPUID_FEAT_ECX_EST)
                log("EST "); // Enhanced SpeedStep.
            if (features2 & CPUID_FEAT_ECX_TM2)
                log("TM2 "); // Thermal Monitor 2.
            if (features2 & CPUID_FEAT_ECX_SSSE3)
                log("SSSE3 "); // Supplemental SSE3 instructions.
            if (features2 & CPUID_FEAT_ECX_CID)
                log("CNXT-ID "); // L1 Context ID.
            if (features2 & CPUID_FEAT_ECX_SDBG)
                log("SDBG "); // Silicon Debug interface.
            if (features2 & CPUID_FEAT_ECX_FMA)
                log("FMA "); // Fused multiply-add.
            if (features2 & CPUID_FEAT_ECX_CX16)
                log("CX16 "); // CMPXCHG16B instruction.
            if (features2 & CPUID_FEAT_ECX_ETPRD)
                log("XTPR "); // Can disable sending task priority messages.
            if (features2 & CPUID_FEAT_ECX_PDCM)
                log("PDCM "); // Perfmon & debug capability.
            if (features2 & CPUID_FEAT_ECX_PCIDE)
                log("PCID "); // Process context identifiers.
            if (features2 & CPUID_FEAT_ECX_DCA)
                log("DCA "); // Direct cache access for DMA writes.
            if (features2 & CPUID_FEAT_ECX_SSE4_1)
                log("SSE4_1 "); // SSE4.1 instructions.
            if (features2 & CPUID_FEAT_ECX_SSE4_2)
                log("SSE4_2 "); // SSE4.2 instructions.
            if (features2 & CPUID_FEAT_ECX_x2APIC)
                log("X2APIC "); // x2APIC support.
            if (features2 & CPUID_FEAT_ECX_MOVBE)
                log("MOVBE "); // MOVBE instruction (big-endian).
            if (features2 & CPUID_FEAT_ECX_POPCNT)
                log("POPCNT "); // POPCNT instruction.
            if (features2 & CPUID_FEAT_ECX_TSC_DEAD)
                log("TSC-DEADLINE "); // APIC supports one-shot operation using a TSC deadline value.
            if (features2 & CPUID_FEAT_ECX_AES)
                log("AES "); // AES instruction set.
            if (features2 & CPUID_FEAT_ECX_XSAVE)
                log("XSAVE "); // XSAVE, XRESTOR, XSETBV, XGETBV.
            if (features2 & CPUID_FEAT_ECX_OSXSAVE)
                log("OSXSAVE "); // XSAVE enabled by OS.
            if (features2 & CPUID_FEAT_ECX_AVX)
                log("AVX "); // Advanced Vector Extensions.
            if (features2 & CPUID_FEAT_ECX_F16C)
                log("F16C "); // F16C (half-precision) FP support.
            if (features2 & CPUID_FEAT_ECX_RDRAND)
                log("RDRND "); // RDRAND (on-chip random number generator) support.
            if (features2 & CPUID_FEAT_ECX_HYPERVISOR)
                log("HYPERVISOR "); // Running on a hypervisor (always 0 on a real CPU, but also with some hypervisors).          
        }

        // Does this processor support extended feature flags?
        if (highest_cpuidfunc >= CPUID_GETEXTENDEDFEATURES) {
            
        }

        // Get highest extended CPUID function supported
        uint32_t highestextended_cpuidfunc = _cpuid_gethighestextendedfunction();
        
        // Does this processor support extended (AMD) feature flags?
        if (highestextended_cpuidfunc >= CPUID_INTELFEATURES) {
            // Only check ones that are not duplicates of the standard feature bits.
            // EDX features.
            uint32_t featurese = _cpuid_getextendedprocessorfeatures();
            if (featurese)
                log("\n");
            if (featurese & CPUID_FEAT_EDX_SYSCALL)
                log("SYSCALL "); // SYSCALL and SYSRET instructions.
            if (featurese & CPUID_FEAT_EDX_MP)
                log("MP "); // Multiprocessor Capable.
            if (featurese & CPUID_FEAT_EDX_NX)
                log("NX "); // NX bit.
            if (featurese & CPUID_FEAT_EDX_MMXEXT)
                log("MMXEXT "); // Extended MMX.
            if (featurese & CPUID_FEAT_EDX_FXSR_OPT)
                log("FXSR_OPT "); // FXSAVE/FXRSTOR optimizations.
            if (featurese & CPUID_FEAT_EDX_PDPE1GB)
                log("PDPE1GB "); // Gibibyte pages.
            if (featurese & CPUID_FEAT_EDX_RDTSCP)
                log("RDTSCP "); // RDTSCP instruction.
            if (featurese & CPUID_FEAT_EDX_LM)
                log("LM "); // Long mode.
            if (featurese & CPUID_FEAT_EDX_3DNOWEXT)
                log("3DNOWEXT "); // Extended 3DNow!
            if (featurese & CPUID_FEAT_EDX_3DNOW)
                log("3DNOW "); // 3DNow!

            // ECX features.
            uint32_t featurese2 = _cpuid_getextendedprocessorfeatures2();
            if (featurese2)
                log("\n");
            if (featurese2 & CPUID_FEAT_ECX_LAHF_LM)
                log("LAHF_LM "); // LAHF/SAHF in long mode.
            if (featurese2 & CPUID_FEAT_ECX_CMP_LEGACY)
                log("CMP_LEGACY "); // Hyperthreading not valid.
            if (featurese2 & CPUID_FEAT_ECX_SVM)
                log("SVM "); // Secure Virtual Machine.
            if (featurese2 & CPUID_FEAT_ECX_EXTAPIC)
                log("EXTAPIC "); // Extended APIC space.
            if (featurese2 & CPUID_FEAT_ECX_CR8_LEGACY)
                log("CR8_LEGACY "); // CR8 in 32-bit mode.
            if (featurese2 & CPUID_FEAT_ECX_ABM)
                log("ABM ");// Advanced bit manipulation (lzcnt and popcnt).
            if (featurese2 & CPUID_FEAT_ECX_SSE4A)
                log("SSE4A "); // SSE4a.
            if (featurese2 & CPUID_FEAT_ECX_MISALIGNSSE)
                log("MISALIGNSSE "); // Misaligned SSE mode.
            if (featurese2 & CPUID_FEAT_ECX_3DNOWPREFETCH)
                log("3DNOWPREFETCH "); // PREFETCH and PREFETCHW instructions.
            if (featurese2 & CPUID_FEAT_ECX_OSVW)
                log("OSVW "); // OS Visible Workaround.
            if (featurese2 & CPUID_FEAT_ECX_IBS)
                log("IBS "); // Instruction Based Sampling.
            if (featurese2 & CPUID_FEAT_ECX_XOP)
                log("XOP "); // XOP instruction set.
            if (featurese2 & CPUID_FEAT_ECX_SKINIT)
                log("SKINIT "); // SKINIT/STGI instructions.
            if (featurese2 & CPUID_FEAT_ECX_WDT)
                log("WDT "); // Watchdog timer.
            if (featurese2 & CPUID_FEAT_ECX_LWP)
                log("LWP "); // Light Weight Profiling.
            if (featurese2 & CPUID_FEAT_ECX_FMA4)
                log("FMA4 "); // 4 operands fused multiply-add.
            if (featurese2 & CPUID_FEAT_ECX_TCE)
                log("TCE "); // Translation Cache Extension.
            if (featurese2 & CPUID_FEAT_ECX_NODEID_MSR)
                log("NODEID_MSR "); // NodeID MSR
            if (featurese2 & CPUID_FEAT_ECX_TBM)
                log("TBM "); // Trailing Bit Manipulation
            if (featurese2 & CPUID_FEAT_ECX_TOPOEXT)
                log("TOPOEXT "); // Topology Extensions.
            if (featurese2 & CPUID_FEAT_ECX_PERFCTR_CORE)
                log("PERFCTR_CORE "); // Core performance counter extensions.
            if (featurese2 & CPUID_FEAT_ECX_PERFCTR_NB)
                log("PERFCTR_NB "); // NB performance counter extensions.
            if (featurese2 & CPUID_FEAT_ECX_BPEXT)
                log("BPEXT "); // Data breakpoint extensions.
            if (featurese2 & CPUID_FEAT_ECX_PTSC)
                log("PTSC "); // Performance TSC.
            if (featurese2 & CPUID_FEAT_ECX_PERFCTR_L2)
                log("PERFCTR_L2 "); // L2I perf counter extensions.
            if (featurese2 & CPUID_FEAT_ECX_MWAITX)
                log("MWAITX "); // MWAIT extensions.
        }
        
        log("\n");

        // Does this processor support the brand string?
        if (highestextended_cpuidfunc >= CPUID_INTELBRANDSTRING) {
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
