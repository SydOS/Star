#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/cpuid.h>

extern uint32_t _cpuid_detect();
extern uint32_t _cpuid_gethighestfunction();
extern uint32_t _cpuid_getsignature();
extern uint32_t _cpuid_getmiscinfo();
extern uint32_t _cpuid_getfeatures_edx();
extern uint32_t _cpuid_getfeatures_ecx();
extern uint32_t _cpuid_getextendedprocessorfeatures_ebx();
extern uint32_t _cpuid_getextendedprocessorfeatures_ecx();
extern uint32_t _cpuid_getextendedprocessorfeatures_edx();
extern uint32_t _cpuid_gethighestextendedfunction();
extern uint32_t _cpuid_geteeprocessorfeatures_edx();
extern uint32_t _cpuid_geteeprocessorfeatures_ecx();

void cpuid_print_capabilities() {
    char temp1[32];

    // Determine if CPUID is even supported.
    // A return value of 0 indicates CPUID is supported.
    if(_cpuid_detect() != 0) {
        // Get highest CPUID function supported
        uint32_t highest_cpuidfunc = _cpuid_gethighestfunction();

        // Print vendor ID if supported (should be).
	    
	    uint32_t cpuvendor[4];
	    cpuid_vendorstring(cpuvendor);
        cpuvendor[3] = '\0';
        kprintf("Processor vendor: %s\n", (char*)cpuvendor);

        // Print processor info and feature bits if supported.
        if (highest_cpuidfunc >= CPUID_GETFEATURES) {
            // Get the processor signature.
            uint32_t signature = _cpuid_getsignature();
            kprintf("Family %u, model %u, stepping %u\n", (signature & 0x00000F00) >> 8,
                (signature & 0x000000F0) >> 4, signature & 0x0000000F);

            // Get processor features.
            kprintf("Features: ");

            // EDX features.
            uint32_t features = _cpuid_getfeatures_edx();
            if (features & CPUID_FEAT_EDX_FPU)
                kprintf("FPU ");
            if (features & CPUID_FEAT_EDX_VME)
                kprintf("VME ");
            if (features & CPUID_FEAT_EDX_DE)
                kprintf("DE ");
            if (features & CPUID_FEAT_EDX_PSE)
                kprintf("PSE ");
            if (features & CPUID_FEAT_EDX_TSC)
                kprintf("TSC ");
            if (features & CPUID_FEAT_EDX_MSR)
                kprintf("MSR ");
            if (features & CPUID_FEAT_EDX_PAE)
                kprintf("PAE ");
            if (features & CPUID_FEAT_EDX_MCE)
                kprintf("MCE ");
            if (features & CPUID_FEAT_EDX_CX8)
                kprintf("CX8 ");
            if (features & CPUID_FEAT_EDX_APIC)
                kprintf("APIC ");
            if (features & CPUID_FEAT_EDX_SEP)
                kprintf("SEP ");
            if (features & CPUID_FEAT_EDX_MTRR)
                kprintf("MTRR ");
            if (features & CPUID_FEAT_EDX_PGE)
                kprintf("PGE ");
            if (features & CPUID_FEAT_EDX_MCA)
                kprintf("MCA ");
            if (features & CPUID_FEAT_EDX_CMOV)
                kprintf("CMOV ");
            if (features & CPUID_FEAT_EDX_PAT)
                kprintf("PAT ");
            if (features & CPUID_FEAT_EDX_PSE36)
                kprintf("PSE36 ");
            if (features & CPUID_FEAT_EDX_PSN)
                kprintf("PSN ");
            if (features & CPUID_FEAT_EDX_CLF)
                kprintf("CLFSH ");
            if (features & CPUID_FEAT_EDX_DTES)
                kprintf("DTES ");
            if (features & CPUID_FEAT_EDX_ACPI)
                kprintf("ACPI ");
            if (features & CPUID_FEAT_EDX_MMX)
                kprintf("MMX ");
            if (features & CPUID_FEAT_EDX_FXSR)
                kprintf("FXSR ");
            if (features & CPUID_FEAT_EDX_SSE)
                kprintf("SSE ");
            if (features & CPUID_FEAT_EDX_SSE2)
                kprintf("SSE2 ");
            if (features & CPUID_FEAT_EDX_SS)
                kprintf("SS ");
            if (features & CPUID_FEAT_EDX_HTT)
                kprintf("HTT ");
            if (features & CPUID_FEAT_EDX_TM1)
                kprintf("TM ");
            if (features & CPUID_FEAT_EDX_IA64)
                kprintf("IA64 ");
            if (features & CPUID_FEAT_EDX_PBE)
                kprintf("PBE ");
            
            // ECX features.     
            uint32_t features2 = _cpuid_getfeatures_ecx();
            if (features2)
                kprintf("\n");
            if (features2 & CPUID_FEAT_ECX_SSE3)
                kprintf("SSE3 ");
            if (features2 & CPUID_FEAT_ECX_PCLMUL)
                kprintf("PCLMULQDQ ");
            if (features2 & CPUID_FEAT_ECX_DTES64)
                kprintf("DTES64 ");
            if (features2 & CPUID_FEAT_ECX_MONITOR)
                kprintf("MONITOR ");
            if (features2 & CPUID_FEAT_ECX_DS_CPL)
                kprintf("DS-CPL ");
            if (features2 & CPUID_FEAT_ECX_VMX)
                kprintf("VMX ");
            if (features2 & CPUID_FEAT_ECX_SMX)
                kprintf("SMX ");
            if (features2 & CPUID_FEAT_ECX_EST)
                kprintf("EST ");
            if (features2 & CPUID_FEAT_ECX_TM2)
                kprintf("TM2 ");
            if (features2 & CPUID_FEAT_ECX_SSSE3)
                kprintf("SSSE3 ");
            if (features2 & CPUID_FEAT_ECX_CID)
                kprintf("CNXT-ID ");
            if (features2 & CPUID_FEAT_ECX_SDBG)
                kprintf("SDBG ");
            if (features2 & CPUID_FEAT_ECX_FMA)
                kprintf("FMA ");
            if (features2 & CPUID_FEAT_ECX_CX16)
                kprintf("CX16 ");
            if (features2 & CPUID_FEAT_ECX_ETPRD)
                kprintf("XTPR ");
            if (features2 & CPUID_FEAT_ECX_PDCM)
                kprintf("PDCM ");
            if (features2 & CPUID_FEAT_ECX_PCIDE)
                kprintf("PCID ");
            if (features2 & CPUID_FEAT_ECX_DCA)
                kprintf("DCA ");
            if (features2 & CPUID_FEAT_ECX_SSE4_1)
                kprintf("SSE4_1 ");
            if (features2 & CPUID_FEAT_ECX_SSE4_2)
                kprintf("SSE4_2 ");
            if (features2 & CPUID_FEAT_ECX_x2APIC)
                kprintf("X2APIC ");
            if (features2 & CPUID_FEAT_ECX_MOVBE)
                kprintf("MOVBE ");
            if (features2 & CPUID_FEAT_ECX_POPCNT)
                kprintf("POPCNT ");
            if (features2 & CPUID_FEAT_ECX_TSC_DEAD)
                kprintf("TSC_DEADLINE "); 
            if (features2 & CPUID_FEAT_ECX_AES)
                kprintf("AES ");
            if (features2 & CPUID_FEAT_ECX_XSAVE)
                kprintf("XSAVE ");
            if (features2 & CPUID_FEAT_ECX_OSXSAVE)
                kprintf("OSXSAVE ");
            if (features2 & CPUID_FEAT_ECX_AVX)
                kprintf("AVX ");
            if (features2 & CPUID_FEAT_ECX_F16C)
                kprintf("F16C ");
            if (features2 & CPUID_FEAT_ECX_RDRAND)
                kprintf("RDRND ");
            if (features2 & CPUID_FEAT_ECX_HYPERVISOR)
                kprintf("HYPERVISOR ");
        }

        // Does this processor support extended feature flags?
        if (highest_cpuidfunc >= CPUID_GETEXTENDEDFEATURES) {
            // EBX features.
            uint32_t featurese_ebx = _cpuid_getextendedprocessorfeatures_ebx();
            if (featurese_ebx)
                kprintf("\n");
            if (featurese_ebx & CPUID_FEAT_EBX_FSGSBASE)
                kprintf("FSGSBASE ");
            if (featurese_ebx & CPUID_FEAT_EBX_TSC_ADJUST)
                kprintf("TSC_ADJUST ");
            if (featurese_ebx & CPUID_FEAT_EBX_SGX)
                kprintf("SGX ");
            if (featurese_ebx & CPUID_FEAT_EBX_BMI1)
                kprintf("BMI1 ");
            if (featurese_ebx & CPUID_FEAT_EBX_HLE)
                kprintf("HLE ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX2)
                kprintf("AVX2 ");
            if (featurese_ebx & CPUID_FEAT_EBX_SMEP)
                kprintf("SMEP ");
            if (featurese_ebx & CPUID_FEAT_EBX_BMI2)
                kprintf("BMI2 ");
            if (featurese_ebx & CPUID_FEAT_EBX_ERMS)
                kprintf("ERMS ");
            if (featurese_ebx & CPUID_FEAT_EBX_INVPCID)
                kprintf("INVPCID ");
            if (featurese_ebx & CPUID_FEAT_EBX_RTM)
                kprintf("RTM ");
            if (featurese_ebx & CPUID_FEAT_EBX_PQM)
                kprintf("PQM ");
            if (featurese_ebx & CPUID_FEAT_EBX_MPX)
                kprintf("MPX ");
            if (featurese_ebx & CPUID_FEAT_EBX_PQE)
                kprintf("PQE ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512)
                kprintf("AVX512 ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512DQ)
                kprintf("AVX512DQ ");
            if (featurese_ebx & CPUID_FEAT_EBX_RDSEED)
                kprintf("RDSEED ");
            if (featurese_ebx & CPUID_FEAT_EBX_ADX)
                kprintf("ADX ");
            if (featurese_ebx & CPUID_FEAT_EBX_SMAP)
                kprintf("SMAP ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512IFMA)
                kprintf("AVX512IFMA ");
            if (featurese_ebx & CPUID_FEAT_EBX_PCOMMIT)
                kprintf("PCOMMIT ");
            if (featurese_ebx & CPUID_FEAT_EBX_CLFLUSHOPT)
                kprintf("CLFLUSHOPT ");
            if (featurese_ebx & CPUID_FEAT_EBX_CLWB)
                kprintf("CLWB ");
            if (featurese_ebx & CPUID_FEAT_EBX_INTELPT)
                kprintf("INTELPT ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512PF)
                kprintf("AVX512PF ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512ER)
                kprintf("AVX512ER ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512CD)
                kprintf("AVX512CD ");
            if (featurese_ebx & CPUID_FEAT_EBX_SHA)
                kprintf("SHA ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512BW)
                kprintf("AVX512BW ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512VL)
                kprintf("AVX512VL ");

            // ECX features.
            uint32_t featurese_ecx = _cpuid_getextendedprocessorfeatures_ecx();
            if (featurese_ecx & CPUID_FEAT_ECX_PREFETCHWT1)
                kprintf("PREFETCHWT1 ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VBMI)
                kprintf("AVX512VBMI ");
            if (featurese_ecx & CPUID_FEAT_ECX_UMIP)
                kprintf("UMIP ");
            if (featurese_ecx & CPUID_FEAT_ECX_PKU)
                kprintf("PKU ");
            if (featurese_ecx & CPUID_FEAT_ECX_OSPKE)
                kprintf("OSPKE ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VBMI2)
                kprintf("AVX512VBMI2 ");
            if (featurese_ecx & CPUID_FEAT_ECX_GFNI)
                kprintf("GFNI ");
            if (featurese_ecx & CPUID_FEAT_ECX_VAES)
                kprintf("VAES ");
            if (featurese_ecx & CPUID_FEAT_ECX_VPCLMULQDQ)
                kprintf("VPCLMULQDQ ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VNNI)
                kprintf("AVX512VNNI ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512BITALG)
                kprintf("AVX512BITALG ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VPOPCNT)
                kprintf("AVX512VPOPCNT ");
            if (featurese_ecx & CPUID_FEAT_ECX_MAWAU)
                kprintf("MAWAU ");
            if (featurese_ecx & CPUID_FEAT_ECX_RDPID)
                kprintf("RDPID ");
            if (featurese_ecx & CPUID_FEAT_ECX_SGX_LC)
                kprintf("SGX_LC ");

            // EDX features.
            uint32_t featurese_edx = _cpuid_getextendedprocessorfeatures_edx();
            if (featurese_edx & CPUID_FEAT_EDX_AVX512_4VNNIW)
                kprintf("AVX512_4VNNIW ");
            if (featurese_edx & CPUID_FEAT_EDX_AVX512_4FMAPS)
                kprintf("AVX512_4FMAPS ");
            if (featurese_edx & CPUID_FEAT_EDX_SPEC_CTRL)
                kprintf("SPEC_CTRL ");
        }

        // Get highest extended CPUID function supported
        uint32_t highestextended_cpuidfunc = _cpuid_gethighestextendedfunction();
        
        // Does this processor support additional extended (AMD) feature flags?
        if (highestextended_cpuidfunc >= CPUID_INTELFEATURES) {
            // Only check ones that are not duplicates of the standard feature bits.
            // EDX features.
            uint32_t featuresee = _cpuid_geteeprocessorfeatures_edx();
            if (featuresee)
                kprintf("\n");
            if (featuresee & CPUID_FEAT_EDX_SYSCALL)
                kprintf("SYSCALL ");
            if (featuresee & CPUID_FEAT_EDX_MP)
                kprintf("MP ");
            if (featuresee & CPUID_FEAT_EDX_NX)
                kprintf("NX ");
            if (featuresee & CPUID_FEAT_EDX_MMXEXT)
                kprintf("MMXEXT ");
            if (featuresee & CPUID_FEAT_EDX_FXSR_OPT)
                kprintf("FXSR_OPT ");
            if (featuresee & CPUID_FEAT_EDX_PDPE1GB)
                kprintf("PDPE1GB ");
            if (featuresee & CPUID_FEAT_EDX_RDTSCP)
                kprintf("RDTSCP ");
            if (featuresee & CPUID_FEAT_EDX_LM)
                kprintf("LM ");
            if (featuresee & CPUID_FEAT_EDX_3DNOWEXT)
                kprintf("3DNOWEXT ");
            if (featuresee & CPUID_FEAT_EDX_3DNOW)
                kprintf("3DNOW ");

            // ECX features.
            uint32_t featuresee2 = _cpuid_geteeprocessorfeatures_ecx();
            if (featuresee2)
                kprintf("\n");
            if (featuresee2 & CPUID_FEAT_ECX_LAHF_LM)
                kprintf("LAHF_LM ");
            if (featuresee2 & CPUID_FEAT_ECX_CMP_LEGACY)
                kprintf("CMP_LEGACY ");
            if (featuresee2 & CPUID_FEAT_ECX_SVM)
                kprintf("SVM ");
            if (featuresee2 & CPUID_FEAT_ECX_EXTAPIC)
                kprintf("EXTAPIC ");
            if (featuresee2 & CPUID_FEAT_ECX_CR8_LEGACY)
                kprintf("CR8_LEGACY ");
            if (featuresee2 & CPUID_FEAT_ECX_ABM)
                kprintf("ABM ");
            if (featuresee2 & CPUID_FEAT_ECX_SSE4A)
                kprintf("SSE4A ");
            if (featuresee2 & CPUID_FEAT_ECX_MISALIGNSSE)
                kprintf("MISALIGNSSE ");
            if (featuresee2 & CPUID_FEAT_ECX_3DNOWPREFETCH)
                kprintf("3DNOWPREFETCH "); 
            if (featuresee2 & CPUID_FEAT_ECX_OSVW)
                kprintf("OSVW ");
            if (featuresee2 & CPUID_FEAT_ECX_IBS)
                kprintf("IBS ");
            if (featuresee2 & CPUID_FEAT_ECX_XOP)
                kprintf("XOP ");
            if (featuresee2 & CPUID_FEAT_ECX_SKINIT)
                kprintf("SKINIT ");
            if (featuresee2 & CPUID_FEAT_ECX_WDT)
                kprintf("WDT ");
            if (featuresee2 & CPUID_FEAT_ECX_LWP)
                kprintf("LWP ");
            if (featuresee2 & CPUID_FEAT_ECX_FMA4)
                kprintf("FMA4 ");
            if (featuresee2 & CPUID_FEAT_ECX_TCE)
                kprintf("TCE ");
            if (featuresee2 & CPUID_FEAT_ECX_NODEID_MSR)
                kprintf("NODEID_MSR ");
            if (featuresee2 & CPUID_FEAT_ECX_TBM)
                kprintf("TBM ");
            if (featuresee2 & CPUID_FEAT_ECX_TOPOEXT)
                kprintf("TOPOEXT ");
            if (featuresee2 & CPUID_FEAT_ECX_PERFCTR_CORE)
                kprintf("PERFCTR_CORE ");
            if (featuresee2 & CPUID_FEAT_ECX_PERFCTR_NB)
                kprintf("PERFCTR_NB ");
            if (featuresee2 & CPUID_FEAT_ECX_BPEXT)
                kprintf("BPEXT ");
            if (featuresee2 & CPUID_FEAT_ECX_PTSC)
                kprintf("PTSC ");
            if (featuresee2 & CPUID_FEAT_ECX_PERFCTR_L2)
                kprintf("PERFCTR_L2 ");
            if (featuresee2 & CPUID_FEAT_ECX_MWAITX)
                kprintf("MWAITX ");
        }
        
        kprintf("\n");

        // Does this processor support the brand string?
        if (highestextended_cpuidfunc >= CPUID_INTELBRANDSTRING) {
            uint32_t cpuid[5];
    	    cpuid_string(CPUID_INTELBRANDSTRING, cpuid);
    	    cpuid[4] = '\0';
    	    kprintf((char*)cpuid);

    	    cpuid_string(CPUID_INTELBRANDSTRINGMORE, cpuid);
    	    cpuid[4] = '\0';
    	    kprintf((char*)cpuid);
    
    	    cpuid_string(CPUID_INTELBRANDSTRINGEND, cpuid);
    	    cpuid[4] = '\0';
    	    kprintf((char*)cpuid);
    	    kprintf("\n");
        }
    }
    else {
        // CPUID is not supported.
        kprintf("CPUID is not supported.\n");
    }
}
