#include <main.h>
#include <tools.h>
#include <logging.h>
#include <driver/cpuid.h>

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
            uint32_t features = _cpuid_getfeatures_edx();
            if (features & CPUID_FEAT_EDX_FPU)
                log("FPU ");
            if (features & CPUID_FEAT_EDX_VME)
                log("VME ");
            if (features & CPUID_FEAT_EDX_DE)
                log("DE ");
            if (features & CPUID_FEAT_EDX_PSE)
                log("PSE ");
            if (features & CPUID_FEAT_EDX_TSC)
                log("TSC ");
            if (features & CPUID_FEAT_EDX_MSR)
                log("MSR ");
            if (features & CPUID_FEAT_EDX_PAE)
                log("PAE ");
            if (features & CPUID_FEAT_EDX_MCE)
                log("MCE ");
            if (features & CPUID_FEAT_EDX_CX8)
                log("CX8 ");
            if (features & CPUID_FEAT_EDX_APIC)
                log("APIC ");
            if (features & CPUID_FEAT_EDX_SEP)
                log("SEP ");
            if (features & CPUID_FEAT_EDX_MTRR)
                log("MTRR ");
            if (features & CPUID_FEAT_EDX_PGE)
                log("PGE ");
            if (features & CPUID_FEAT_EDX_MCA)
                log("MCA ");
            if (features & CPUID_FEAT_EDX_CMOV)
                log("CMOV ");
            if (features & CPUID_FEAT_EDX_PAT)
                log("PAT ");
            if (features & CPUID_FEAT_EDX_PSE36)
                log("PSE36 ");
            if (features & CPUID_FEAT_EDX_PSN)
                log("PSN ");
            if (features & CPUID_FEAT_EDX_CLF)
                log("CLFSH ");
            if (features & CPUID_FEAT_EDX_DTES)
                log("DTES ");
            if (features & CPUID_FEAT_EDX_ACPI)
                log("ACPI ");
            if (features & CPUID_FEAT_EDX_MMX)
                log("MMX ");
            if (features & CPUID_FEAT_EDX_FXSR)
                log("FXSR ");
            if (features & CPUID_FEAT_EDX_SSE)
                log("SSE ");
            if (features & CPUID_FEAT_EDX_SSE2)
                log("SSE2 ");
            if (features & CPUID_FEAT_EDX_SS)
                log("SS ");
            if (features & CPUID_FEAT_EDX_HTT)
                log("HTT ");
            if (features & CPUID_FEAT_EDX_TM1)
                log("TM ");
            if (features & CPUID_FEAT_EDX_IA64)
                log("IA64 ");
            if (features & CPUID_FEAT_EDX_PBE)
                log("PBE ");
            
            // ECX features.     
            uint32_t features2 = _cpuid_getfeatures_ecx();
            if (features2)
                log("\n");
            if (features2 & CPUID_FEAT_ECX_SSE3)
                log("SSE3 ");
            if (features2 & CPUID_FEAT_ECX_PCLMUL)
                log("PCLMULQDQ ");
            if (features2 & CPUID_FEAT_ECX_DTES64)
                log("DTES64 ");
            if (features2 & CPUID_FEAT_ECX_MONITOR)
                log("MONITOR ");
            if (features2 & CPUID_FEAT_ECX_DS_CPL)
                log("DS-CPL ");
            if (features2 & CPUID_FEAT_ECX_VMX)
                log("VMX ");
            if (features2 & CPUID_FEAT_ECX_SMX)
                log("SMX ");
            if (features2 & CPUID_FEAT_ECX_EST)
                log("EST ");
            if (features2 & CPUID_FEAT_ECX_TM2)
                log("TM2 ");
            if (features2 & CPUID_FEAT_ECX_SSSE3)
                log("SSSE3 ");
            if (features2 & CPUID_FEAT_ECX_CID)
                log("CNXT-ID ");
            if (features2 & CPUID_FEAT_ECX_SDBG)
                log("SDBG ");
            if (features2 & CPUID_FEAT_ECX_FMA)
                log("FMA ");
            if (features2 & CPUID_FEAT_ECX_CX16)
                log("CX16 ");
            if (features2 & CPUID_FEAT_ECX_ETPRD)
                log("XTPR ");
            if (features2 & CPUID_FEAT_ECX_PDCM)
                log("PDCM ");
            if (features2 & CPUID_FEAT_ECX_PCIDE)
                log("PCID ");
            if (features2 & CPUID_FEAT_ECX_DCA)
                log("DCA ");
            if (features2 & CPUID_FEAT_ECX_SSE4_1)
                log("SSE4_1 ");
            if (features2 & CPUID_FEAT_ECX_SSE4_2)
                log("SSE4_2 ");
            if (features2 & CPUID_FEAT_ECX_x2APIC)
                log("X2APIC ");
            if (features2 & CPUID_FEAT_ECX_MOVBE)
                log("MOVBE ");
            if (features2 & CPUID_FEAT_ECX_POPCNT)
                log("POPCNT ");
            if (features2 & CPUID_FEAT_ECX_TSC_DEAD)
                log("TSC_DEADLINE "); 
            if (features2 & CPUID_FEAT_ECX_AES)
                log("AES ");
            if (features2 & CPUID_FEAT_ECX_XSAVE)
                log("XSAVE ");
            if (features2 & CPUID_FEAT_ECX_OSXSAVE)
                log("OSXSAVE ");
            if (features2 & CPUID_FEAT_ECX_AVX)
                log("AVX ");
            if (features2 & CPUID_FEAT_ECX_F16C)
                log("F16C ");
            if (features2 & CPUID_FEAT_ECX_RDRAND)
                log("RDRND ");
            if (features2 & CPUID_FEAT_ECX_HYPERVISOR)
                log("HYPERVISOR ");
        }

        // Does this processor support extended feature flags?
        if (highest_cpuidfunc >= CPUID_GETEXTENDEDFEATURES) {
            // EBX features.
            uint32_t featurese_ebx = _cpuid_getextendedprocessorfeatures_ebx();
            if (featurese_ebx)
                log("\n");
            if (featurese_ebx & CPUID_FEAT_EBX_FSGSBASE)
                log("FSGSBASE ");
            if (featurese_ebx & CPUID_FEAT_EBX_TSC_ADJUST)
                log("TSC_ADJUST ");
            if (featurese_ebx & CPUID_FEAT_EBX_SGX)
                log("SGX ");
            if (featurese_ebx & CPUID_FEAT_EBX_BMI1)
                log("BMI1 ");
            if (featurese_ebx & CPUID_FEAT_EBX_HLE)
                log("HLE ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX2)
                log("AVX2 ");
            if (featurese_ebx & CPUID_FEAT_EBX_SMEP)
                log("SMEP ");
            if (featurese_ebx & CPUID_FEAT_EBX_BMI2)
                log("BMI2 ");
            if (featurese_ebx & CPUID_FEAT_EBX_ERMS)
                log("ERMS ");
            if (featurese_ebx & CPUID_FEAT_EBX_INVPCID)
                log("INVPCID ");
            if (featurese_ebx & CPUID_FEAT_EBX_RTM)
                log("RTM ");
            if (featurese_ebx & CPUID_FEAT_EBX_PQM)
                log("PQM ");
            if (featurese_ebx & CPUID_FEAT_EBX_MPX)
                log("MPX ");
            if (featurese_ebx & CPUID_FEAT_EBX_PQE)
                log("PQE ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512)
                log("AVX512 ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512DQ)
                log("AVX512DQ ");
            if (featurese_ebx & CPUID_FEAT_EBX_RDSEED)
                log("RDSEED ");
            if (featurese_ebx & CPUID_FEAT_EBX_ADX)
                log("ADX ");
            if (featurese_ebx & CPUID_FEAT_EBX_SMAP)
                log("SMAP ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512IFMA)
                log("AVX512IFMA ");
            if (featurese_ebx & CPUID_FEAT_EBX_PCOMMIT)
                log("PCOMMIT ");
            if (featurese_ebx & CPUID_FEAT_EBX_CLFLUSHOPT)
                log("CLFLUSHOPT ");
            if (featurese_ebx & CPUID_FEAT_EBX_CLWB)
                log("CLWB ");
            if (featurese_ebx & CPUID_FEAT_EBX_INTELPT)
                log("INTELPT ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512PF)
                log("AVX512PF ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512ER)
                log("AVX512ER ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512CD)
                log("AVX512CD ");
            if (featurese_ebx & CPUID_FEAT_EBX_SHA)
                log("SHA ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512BW)
                log("AVX512BW ");
            if (featurese_ebx & CPUID_FEAT_EBX_AVX512VL)
                log("AVX512VL ");

            // ECX features.
            uint32_t featurese_ecx = _cpuid_getextendedprocessorfeatures_ecx();
            if (featurese_ecx & CPUID_FEAT_ECX_PREFETCHWT1)
                log("PREFETCHWT1 ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VBMI)
                log("AVX512VBMI ");
            if (featurese_ecx & CPUID_FEAT_ECX_UMIP)
                log("UMIP ");
            if (featurese_ecx & CPUID_FEAT_ECX_PKU)
                log("PKU ");
            if (featurese_ecx & CPUID_FEAT_ECX_OSPKE)
                log("OSPKE ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VBMI2)
                log("AVX512VBMI2 ");
            if (featurese_ecx & CPUID_FEAT_ECX_GFNI)
                log("GFNI ");
            if (featurese_ecx & CPUID_FEAT_ECX_VAES)
                log("VAES ");
            if (featurese_ecx & CPUID_FEAT_ECX_VPCLMULQDQ)
                log("VPCLMULQDQ ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VNNI)
                log("AVX512VNNI ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512BITALG)
                log("AVX512BITALG ");
            if (featurese_ecx & CPUID_FEAT_ECX_AVX512VPOPCNT)
                log("AVX512VPOPCNT ");
            if (featurese_ecx & CPUID_FEAT_ECX_MAWAU)
                log("MAWAU ");
            if (featurese_ecx & CPUID_FEAT_ECX_RDPID)
                log("RDPID ");
            if (featurese_ecx & CPUID_FEAT_ECX_SGX_LC)
                log("SGX_LC ");

            // EDX features.
            uint32_t featurese_edx = _cpuid_getextendedprocessorfeatures_edx();
            if (featurese_edx & CPUID_FEAT_EDX_AVX512_4VNNIW)
                log("AVX512_4VNNIW ");
            if (featurese_edx & CPUID_FEAT_EDX_AVX512_4FMAPS)
                log("AVX512_4FMAPS ");
            if (featurese_edx & CPUID_FEAT_EDX_SPEC_CTRL)
                log("SPEC_CTRL ");
        }

        // Get highest extended CPUID function supported
        uint32_t highestextended_cpuidfunc = _cpuid_gethighestextendedfunction();
        
        // Does this processor support additional extended (AMD) feature flags?
        if (highestextended_cpuidfunc >= CPUID_INTELFEATURES) {
            // Only check ones that are not duplicates of the standard feature bits.
            // EDX features.
            uint32_t featuresee = _cpuid_geteeprocessorfeatures_edx();
            if (featuresee)
                log("\n");
            if (featuresee & CPUID_FEAT_EDX_SYSCALL)
                log("SYSCALL ");
            if (featuresee & CPUID_FEAT_EDX_MP)
                log("MP ");
            if (featuresee & CPUID_FEAT_EDX_NX)
                log("NX ");
            if (featuresee & CPUID_FEAT_EDX_MMXEXT)
                log("MMXEXT ");
            if (featuresee & CPUID_FEAT_EDX_FXSR_OPT)
                log("FXSR_OPT ");
            if (featuresee & CPUID_FEAT_EDX_PDPE1GB)
                log("PDPE1GB ");
            if (featuresee & CPUID_FEAT_EDX_RDTSCP)
                log("RDTSCP ");
            if (featuresee & CPUID_FEAT_EDX_LM)
                log("LM ");
            if (featuresee & CPUID_FEAT_EDX_3DNOWEXT)
                log("3DNOWEXT ");
            if (featuresee & CPUID_FEAT_EDX_3DNOW)
                log("3DNOW ");

            // ECX features.
            uint32_t featuresee2 = _cpuid_geteeprocessorfeatures_ecx();
            if (featuresee2)
                log("\n");
            if (featuresee2 & CPUID_FEAT_ECX_LAHF_LM)
                log("LAHF_LM ");
            if (featuresee2 & CPUID_FEAT_ECX_CMP_LEGACY)
                log("CMP_LEGACY ");
            if (featuresee2 & CPUID_FEAT_ECX_SVM)
                log("SVM ");
            if (featuresee2 & CPUID_FEAT_ECX_EXTAPIC)
                log("EXTAPIC ");
            if (featuresee2 & CPUID_FEAT_ECX_CR8_LEGACY)
                log("CR8_LEGACY ");
            if (featuresee2 & CPUID_FEAT_ECX_ABM)
                log("ABM ");
            if (featuresee2 & CPUID_FEAT_ECX_SSE4A)
                log("SSE4A ");
            if (featuresee2 & CPUID_FEAT_ECX_MISALIGNSSE)
                log("MISALIGNSSE ");
            if (featuresee2 & CPUID_FEAT_ECX_3DNOWPREFETCH)
                log("3DNOWPREFETCH "); 
            if (featuresee2 & CPUID_FEAT_ECX_OSVW)
                log("OSVW ");
            if (featuresee2 & CPUID_FEAT_ECX_IBS)
                log("IBS ");
            if (featuresee2 & CPUID_FEAT_ECX_XOP)
                log("XOP ");
            if (featuresee2 & CPUID_FEAT_ECX_SKINIT)
                log("SKINIT ");
            if (featuresee2 & CPUID_FEAT_ECX_WDT)
                log("WDT ");
            if (featuresee2 & CPUID_FEAT_ECX_LWP)
                log("LWP ");
            if (featuresee2 & CPUID_FEAT_ECX_FMA4)
                log("FMA4 ");
            if (featuresee2 & CPUID_FEAT_ECX_TCE)
                log("TCE ");
            if (featuresee2 & CPUID_FEAT_ECX_NODEID_MSR)
                log("NODEID_MSR ");
            if (featuresee2 & CPUID_FEAT_ECX_TBM)
                log("TBM ");
            if (featuresee2 & CPUID_FEAT_ECX_TOPOEXT)
                log("TOPOEXT ");
            if (featuresee2 & CPUID_FEAT_ECX_PERFCTR_CORE)
                log("PERFCTR_CORE ");
            if (featuresee2 & CPUID_FEAT_ECX_PERFCTR_NB)
                log("PERFCTR_NB ");
            if (featuresee2 & CPUID_FEAT_ECX_BPEXT)
                log("BPEXT ");
            if (featuresee2 & CPUID_FEAT_ECX_PTSC)
                log("PTSC ");
            if (featuresee2 & CPUID_FEAT_ECX_PERFCTR_L2)
                log("PERFCTR_L2 ");
            if (featuresee2 & CPUID_FEAT_ECX_MWAITX)
                log("MWAITX ");
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
