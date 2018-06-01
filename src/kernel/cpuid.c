/*
 * File: cpuid.c
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/cpuid.h>

extern uint32_t _cpuid_detect();
static bool cpuidChecked = false;
static bool cpuidSupported = false;

static bool cpuid_function_supported(uint32_t function) {
    // Determine if CPUID is even supported.
    if (!cpuidChecked)
        cpuidSupported = (_cpuid_detect() != 0);
    if (!cpuidSupported)
        return false;

    // Check if function is supported.
    uint32_t highestFunction = 0;
    if (function >= CPUID_INTELEXTENDED)
        asm volatile ("cpuid" : "=a"(highestFunction) : "a"(CPUID_INTELEXTENDED), "b"(0), "c"(0), "d"(0));
    else
        asm volatile ("cpuid" : "=a"(highestFunction) : "a"(CPUID_GETVENDORSTRING), "b"(0), "c"(0), "d"(0));
    return highestFunction >= function;
}

// Query CPUID.
bool cpuid_query(uint32_t function, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) { 
    // Check if function is supported.
    if (!cpuid_function_supported(function))
        return false;

    // Execute CPUID function.
    asm volatile ("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) :
                            "a"(function), "b"(0), "c"(0), "d"(0));
    return true;
}

// Print CPUID capabilities.
void cpuid_print_capabilities() {
    uint32_t eax, ebx, ecx, edx;

    // Get vendor string.
    if (cpuid_query(CPUID_GETVENDORSTRING, &eax, &ebx, &ecx, &edx)) {
        uint32_t vendor[4];
        vendor[0] = ebx;
        vendor[1] = edx;
        vendor[2] = ecx;
        vendor[3] = '\0';
        kprintf("Processor vendor: %s\n", (char*)vendor);
    }

    // Print processor info and feature bits.
    if (cpuid_query(CPUID_GETFEATURES, &eax, &ebx, &ecx, &edx)) {
        // Get the processor signature.
        kprintf("Family %u, model %u, stepping %u\n", CPUID_FAMILY(eax), CPUID_MODEL(eax), CPUID_STEPPING(eax));
        kprintf("Features: ");

        // EDX features.
        if (edx & CPUID_FEAT_EDX_FPU)
            kprintf("FPU ");
        if (edx & CPUID_FEAT_EDX_VME)
            kprintf("VME ");
        if (edx & CPUID_FEAT_EDX_DE)
            kprintf("DE ");
        if (edx & CPUID_FEAT_EDX_PSE)
            kprintf("PSE ");
        if (edx & CPUID_FEAT_EDX_TSC)
            kprintf("TSC ");
        if (edx & CPUID_FEAT_EDX_MSR)
            kprintf("MSR ");
        if (edx & CPUID_FEAT_EDX_PAE)
            kprintf("PAE ");
        if (edx & CPUID_FEAT_EDX_MCE)
            kprintf("MCE ");
        if (edx & CPUID_FEAT_EDX_CX8)
            kprintf("CX8 ");
        if (edx & CPUID_FEAT_EDX_APIC)
            kprintf("APIC ");
        if (edx & CPUID_FEAT_EDX_SEP)
            kprintf("SEP ");
        if (edx & CPUID_FEAT_EDX_MTRR)
            kprintf("MTRR ");
        if (edx & CPUID_FEAT_EDX_PGE)
            kprintf("PGE ");
        if (edx & CPUID_FEAT_EDX_MCA)
            kprintf("MCA ");
        if (edx & CPUID_FEAT_EDX_CMOV)
            kprintf("CMOV ");
        if (edx & CPUID_FEAT_EDX_PAT)
            kprintf("PAT ");
        if (edx & CPUID_FEAT_EDX_PSE36)
            kprintf("PSE36 ");
        if (edx & CPUID_FEAT_EDX_PSN)
            kprintf("PSN ");
        if (edx & CPUID_FEAT_EDX_CLF)
            kprintf("CLFSH ");
        if (edx & CPUID_FEAT_EDX_DTES)
            kprintf("DTES ");
        if (edx & CPUID_FEAT_EDX_ACPI)
            kprintf("ACPI ");
        if (edx & CPUID_FEAT_EDX_MMX)
            kprintf("MMX ");
        if (edx & CPUID_FEAT_EDX_FXSR)
            kprintf("FXSR ");
        if (edx & CPUID_FEAT_EDX_SSE)
            kprintf("SSE ");
        if (edx & CPUID_FEAT_EDX_SSE2)
            kprintf("SSE2 ");
        if (edx & CPUID_FEAT_EDX_SS)
            kprintf("SS ");
        if (edx & CPUID_FEAT_EDX_HTT)
            kprintf("HTT ");
        if (edx & CPUID_FEAT_EDX_TM1)
            kprintf("TM ");
        if (edx & CPUID_FEAT_EDX_IA64)
            kprintf("IA64 ");
        if (edx & CPUID_FEAT_EDX_PBE)
            kprintf("PBE ");
        
        // ECX features.     
        if (ecx)
            kprintf("\n");
        if (ecx & CPUID_FEAT_ECX_SSE3)
            kprintf("SSE3 ");
        if (ecx & CPUID_FEAT_ECX_PCLMUL)
            kprintf("PCLMULQDQ ");
        if (ecx & CPUID_FEAT_ECX_DTES64)
            kprintf("DTES64 ");
        if (ecx & CPUID_FEAT_ECX_MONITOR)
            kprintf("MONITOR ");
        if (ecx & CPUID_FEAT_ECX_DS_CPL)
            kprintf("DS-CPL ");
        if (ecx & CPUID_FEAT_ECX_VMX)
            kprintf("VMX ");
        if (ecx & CPUID_FEAT_ECX_SMX)
            kprintf("SMX ");
        if (ecx & CPUID_FEAT_ECX_EST)
            kprintf("EST ");
        if (ecx & CPUID_FEAT_ECX_TM2)
            kprintf("TM2 ");
        if (ecx & CPUID_FEAT_ECX_SSSE3)
            kprintf("SSSE3 ");
        if (ecx & CPUID_FEAT_ECX_CID)
            kprintf("CNXT-ID ");
        if (ecx & CPUID_FEAT_ECX_SDBG)
            kprintf("SDBG ");
        if (ecx & CPUID_FEAT_ECX_FMA)
            kprintf("FMA ");
        if (ecx & CPUID_FEAT_ECX_CX16)
            kprintf("CX16 ");
        if (ecx & CPUID_FEAT_ECX_ETPRD)
            kprintf("XTPR ");
        if (ecx & CPUID_FEAT_ECX_PDCM)
            kprintf("PDCM ");
        if (ecx & CPUID_FEAT_ECX_PCIDE)
            kprintf("PCID ");
        if (ecx & CPUID_FEAT_ECX_DCA)
            kprintf("DCA ");
        if (ecx & CPUID_FEAT_ECX_SSE4_1)
            kprintf("SSE4_1 ");
        if (ecx & CPUID_FEAT_ECX_SSE4_2)
            kprintf("SSE4_2 ");
        if (ecx & CPUID_FEAT_ECX_x2APIC)
            kprintf("X2APIC ");
        if (ecx & CPUID_FEAT_ECX_MOVBE)
            kprintf("MOVBE ");
        if (ecx & CPUID_FEAT_ECX_POPCNT)
            kprintf("POPCNT ");
        if (ecx & CPUID_FEAT_ECX_TSC_DEAD)
            kprintf("TSC_DEADLINE "); 
        if (ecx & CPUID_FEAT_ECX_AES)
            kprintf("AES ");
        if (ecx & CPUID_FEAT_ECX_XSAVE)
            kprintf("XSAVE ");
        if (ecx & CPUID_FEAT_ECX_OSXSAVE)
            kprintf("OSXSAVE ");
        if (ecx & CPUID_FEAT_ECX_AVX)
            kprintf("AVX ");
        if (ecx & CPUID_FEAT_ECX_F16C)
            kprintf("F16C ");
        if (ecx & CPUID_FEAT_ECX_RDRAND)
            kprintf("RDRND ");
        if (ecx & CPUID_FEAT_ECX_HYPERVISOR)
            kprintf("HYPERVISOR ");
    }

    // Does this processor support extended feature flags?
    if (cpuid_query(CPUID_GETEXTENDEDFEATURES, &eax, &ebx, &ecx, &edx)) {
        // EBX features.
        if (ebx)
            kprintf("\n");
        if (ebx & CPUID_FEAT_EBX_FSGSBASE)
            kprintf("FSGSBASE ");
        if (ebx & CPUID_FEAT_EBX_TSC_ADJUST)
            kprintf("TSC_ADJUST ");
        if (ebx & CPUID_FEAT_EBX_SGX)
            kprintf("SGX ");
        if (ebx & CPUID_FEAT_EBX_BMI1)
            kprintf("BMI1 ");
        if (ebx & CPUID_FEAT_EBX_HLE)
            kprintf("HLE ");
        if (ebx & CPUID_FEAT_EBX_AVX2)
            kprintf("AVX2 ");
        if (ebx & CPUID_FEAT_EBX_SMEP)
            kprintf("SMEP ");
        if (ebx & CPUID_FEAT_EBX_BMI2)
            kprintf("BMI2 ");
        if (ebx & CPUID_FEAT_EBX_ERMS)
            kprintf("ERMS ");
        if (ebx & CPUID_FEAT_EBX_INVPCID)
            kprintf("INVPCID ");
        if (ebx & CPUID_FEAT_EBX_RTM)
            kprintf("RTM ");
        if (ebx & CPUID_FEAT_EBX_PQM)
            kprintf("PQM ");
        if (ebx & CPUID_FEAT_EBX_MPX)
            kprintf("MPX ");
        if (ebx & CPUID_FEAT_EBX_PQE)
            kprintf("PQE ");
        if (ebx & CPUID_FEAT_EBX_AVX512)
            kprintf("AVX512 ");
        if (ebx & CPUID_FEAT_EBX_AVX512DQ)
            kprintf("AVX512DQ ");
        if (ebx & CPUID_FEAT_EBX_RDSEED)
            kprintf("RDSEED ");
        if (ebx & CPUID_FEAT_EBX_ADX)
            kprintf("ADX ");
        if (ebx & CPUID_FEAT_EBX_SMAP)
            kprintf("SMAP ");
        if (ebx & CPUID_FEAT_EBX_AVX512IFMA)
            kprintf("AVX512IFMA ");
        if (ebx & CPUID_FEAT_EBX_PCOMMIT)
            kprintf("PCOMMIT ");
        if (ebx & CPUID_FEAT_EBX_CLFLUSHOPT)
            kprintf("CLFLUSHOPT ");
        if (ebx & CPUID_FEAT_EBX_CLWB)
            kprintf("CLWB ");
        if (ebx & CPUID_FEAT_EBX_INTELPT)
            kprintf("INTELPT ");
        if (ebx & CPUID_FEAT_EBX_AVX512PF)
            kprintf("AVX512PF ");
        if (ebx & CPUID_FEAT_EBX_AVX512ER)
            kprintf("AVX512ER ");
        if (ebx & CPUID_FEAT_EBX_AVX512CD)
            kprintf("AVX512CD ");
        if (ebx & CPUID_FEAT_EBX_SHA)
            kprintf("SHA ");
        if (ebx & CPUID_FEAT_EBX_AVX512BW)
            kprintf("AVX512BW ");
        if (ebx & CPUID_FEAT_EBX_AVX512VL)
            kprintf("AVX512VL ");

        // ECX features.
        if (ecx & CPUID_FEAT_ECX_PREFETCHWT1)
            kprintf("PREFETCHWT1 ");
        if (ecx & CPUID_FEAT_ECX_AVX512VBMI)
            kprintf("AVX512VBMI ");
        if (ecx & CPUID_FEAT_ECX_UMIP)
            kprintf("UMIP ");
        if (ecx & CPUID_FEAT_ECX_PKU)
            kprintf("PKU ");
        if (ecx & CPUID_FEAT_ECX_OSPKE)
            kprintf("OSPKE ");
        if (ecx & CPUID_FEAT_ECX_AVX512VBMI2)
            kprintf("AVX512VBMI2 ");
        if (ecx & CPUID_FEAT_ECX_GFNI)
            kprintf("GFNI ");
        if (ecx & CPUID_FEAT_ECX_VAES)
            kprintf("VAES ");
        if (ecx & CPUID_FEAT_ECX_VPCLMULQDQ)
            kprintf("VPCLMULQDQ ");
        if (ecx & CPUID_FEAT_ECX_AVX512VNNI)
            kprintf("AVX512VNNI ");
        if (ecx & CPUID_FEAT_ECX_AVX512BITALG)
            kprintf("AVX512BITALG ");
        if (ecx & CPUID_FEAT_ECX_AVX512VPOPCNT)
            kprintf("AVX512VPOPCNT ");
        if (ecx & CPUID_FEAT_ECX_MAWAU)
            kprintf("MAWAU ");
        if (ecx & CPUID_FEAT_ECX_RDPID)
            kprintf("RDPID ");
        if (ecx & CPUID_FEAT_ECX_SGX_LC)
            kprintf("SGX_LC ");

        // EDX features.
        if (edx & CPUID_FEAT_EDX_AVX512_4VNNIW)
            kprintf("AVX512_4VNNIW ");
        if (edx & CPUID_FEAT_EDX_AVX512_4FMAPS)
            kprintf("AVX512_4FMAPS ");
        if (edx & CPUID_FEAT_EDX_SPEC_CTRL)
            kprintf("SPEC_CTRL ");
    }
        
    // Does this processor support additional extended (AMD) feature flags?
    if (cpuid_query(CPUID_INTELFEATURES, &eax, &ebx, &ecx, &edx)) {
        // Only check ones that are not duplicates of the standard feature bits.
        // EDX features.
        if (edx)
            kprintf("\n");
        if (edx & CPUID_FEAT_EDX_SYSCALL)
            kprintf("SYSCALL ");
        if (edx & CPUID_FEAT_EDX_MP)
            kprintf("MP ");
        if (edx & CPUID_FEAT_EDX_NX)
            kprintf("NX ");
        if (edx & CPUID_FEAT_EDX_MMXEXT)
            kprintf("MMXEXT ");
        if (edx & CPUID_FEAT_EDX_FXSR_OPT)
            kprintf("FXSR_OPT ");
        if (edx & CPUID_FEAT_EDX_PDPE1GB)
            kprintf("PDPE1GB ");
        if (edx & CPUID_FEAT_EDX_RDTSCP)
            kprintf("RDTSCP ");
        if (edx & CPUID_FEAT_EDX_LM)
            kprintf("LM ");
        if (edx & CPUID_FEAT_EDX_3DNOWEXT)
            kprintf("3DNOWEXT ");
        if (edx & CPUID_FEAT_EDX_3DNOW)
            kprintf("3DNOW ");

        // ECX features.
        if (ecx)
            kprintf("\n");
        if (ecx & CPUID_FEAT_ECX_LAHF_LM)
            kprintf("LAHF_LM ");
        if (ecx & CPUID_FEAT_ECX_CMP_LEGACY)
            kprintf("CMP_LEGACY ");
        if (ecx & CPUID_FEAT_ECX_SVM)
            kprintf("SVM ");
        if (ecx & CPUID_FEAT_ECX_EXTAPIC)
            kprintf("EXTAPIC ");
        if (ecx & CPUID_FEAT_ECX_CR8_LEGACY)
            kprintf("CR8_LEGACY ");
        if (ecx & CPUID_FEAT_ECX_ABM)
            kprintf("ABM ");
        if (ecx & CPUID_FEAT_ECX_SSE4A)
            kprintf("SSE4A ");
        if (ecx & CPUID_FEAT_ECX_MISALIGNSSE)
            kprintf("MISALIGNSSE ");
        if (ecx & CPUID_FEAT_ECX_3DNOWPREFETCH)
            kprintf("3DNOWPREFETCH "); 
        if (ecx & CPUID_FEAT_ECX_OSVW)
            kprintf("OSVW ");
        if (ecx & CPUID_FEAT_ECX_IBS)
            kprintf("IBS ");
        if (ecx & CPUID_FEAT_ECX_XOP)
            kprintf("XOP ");
        if (ecx & CPUID_FEAT_ECX_SKINIT)
            kprintf("SKINIT ");
        if (ecx & CPUID_FEAT_ECX_WDT)
            kprintf("WDT ");
        if (ecx & CPUID_FEAT_ECX_LWP)
            kprintf("LWP ");
        if (ecx & CPUID_FEAT_ECX_FMA4)
            kprintf("FMA4 ");
        if (ecx & CPUID_FEAT_ECX_TCE)
            kprintf("TCE ");
        if (ecx & CPUID_FEAT_ECX_NODEID_MSR)
            kprintf("NODEID_MSR ");
        if (ecx & CPUID_FEAT_ECX_TBM)
            kprintf("TBM ");
        if (ecx & CPUID_FEAT_ECX_TOPOEXT)
            kprintf("TOPOEXT ");
        if (ecx & CPUID_FEAT_ECX_PERFCTR_CORE)
            kprintf("PERFCTR_CORE ");
        if (ecx & CPUID_FEAT_ECX_PERFCTR_NB)
            kprintf("PERFCTR_NB ");
        if (ecx & CPUID_FEAT_ECX_BPEXT)
            kprintf("BPEXT ");
        if (ecx & CPUID_FEAT_ECX_PTSC)
            kprintf("PTSC ");
        if (ecx & CPUID_FEAT_ECX_PERFCTR_L2)
            kprintf("PERFCTR_L2 ");
        if (ecx & CPUID_FEAT_ECX_MWAITX)
            kprintf("MWAITX ");
    }  
    kprintf("\n");

    // Does this processor support the brand string?
    if (cpuid_query(CPUID_INTELBRANDSTRING, &eax, &ebx, &ecx, &edx)) {
        uint32_t string1[] = { eax, ebx, ecx, edx, '\0' };
        kprintf((char*)string1);

        cpuid_query(CPUID_INTELBRANDSTRINGMORE, &eax, &ebx, &ecx, &edx);
        uint32_t string2[] = { eax, ebx, ecx, edx, '\0' };
        kprintf((char*)string2);

        cpuid_query(CPUID_INTELBRANDSTRINGEND, &eax, &ebx, &ecx, &edx);
        uint32_t string3[] = { eax, ebx, ecx, edx, '\0' };
        kprintf("%s\n", (char*)string3);
    }
}
