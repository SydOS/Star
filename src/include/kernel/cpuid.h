/*
 * File: cpuid.h
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

#ifndef CPUID_H
#define CPUID_H

#define CPUID_STEPPING(value) (value & 0xF)
#define CPUID_MODEL(value)    ((value & 0xF0) >> 4)
#define CPUID_FAMILY(value)   ((value & 0xF00) >> 8)

enum cpuid_requests {
  CPUID_GETVENDORSTRING,
  CPUID_GETFEATURES,
  CPUID_GETTLB,
  CPUID_GETSERIAL,
  CPUID_GETTHREAD,
  CPUID_GETEXTENDEDFEATURES,
 
  CPUID_INTELEXTENDED=0x80000000,
  CPUID_INTELFEATURES,
  CPUID_INTELBRANDSTRING,
  CPUID_INTELBRANDSTRINGMORE,
  CPUID_INTELBRANDSTRINGEND,
};

enum {
    // Standard features.
    CPUID_FEAT_EDX_FPU          = 1 << 0,  // Onboard x87 FPU.
    CPUID_FEAT_EDX_VME          = 1 << 1,  // Virtual 8086 mode extensions.
    CPUID_FEAT_EDX_DE           = 1 << 2,  // Debugging extensions.
    CPUID_FEAT_EDX_PSE          = 1 << 3,  // Page Size Extension.
    CPUID_FEAT_EDX_TSC          = 1 << 4,  // Time Stamp Counter.
    CPUID_FEAT_EDX_MSR          = 1 << 5,  // Model-specific registers.
    CPUID_FEAT_EDX_PAE          = 1 << 6,  // Physical Address Extension.
    CPUID_FEAT_EDX_MCE          = 1 << 7,  // Machine Check Exception.
    CPUID_FEAT_EDX_CX8          = 1 << 8,  // CMPXCHG8 (compare-and-swap) instruction.
    CPUID_FEAT_EDX_APIC         = 1 << 9,  // Onboard Advanced Programmable Interrupt Controller.
    CPUID_FEAT_EDX_SEP          = 1 << 11, // SYSENTER and SYSEXIT instructions.
    CPUID_FEAT_EDX_MTRR         = 1 << 12, // Memory Type Range Registers.
    CPUID_FEAT_EDX_PGE          = 1 << 13, // Page Global Enable bit in CR4.
    CPUID_FEAT_EDX_MCA          = 1 << 14, // Machine check architecture.
    CPUID_FEAT_EDX_CMOV         = 1 << 15, // Conditional move and FCMOV instructions.
    CPUID_FEAT_EDX_PAT          = 1 << 16, // Page Attribute Table.
    CPUID_FEAT_EDX_PSE36        = 1 << 17, // 36-bit page size extension.
    CPUID_FEAT_EDX_PSN          = 1 << 18, // Processor Serial Number.
    CPUID_FEAT_EDX_CLF          = 1 << 19, // CLFLUSH instruction.
    CPUID_FEAT_EDX_DTES         = 1 << 21, // Debug store: save trace of executed jumps.
    CPUID_FEAT_EDX_ACPI         = 1 << 22, // Onboard thermal control MSRs for ACPI.
    CPUID_FEAT_EDX_MMX          = 1 << 23, // MMX instructions.
    CPUID_FEAT_EDX_FXSR         = 1 << 24, // FXSAVE, FXRESTOR instructions.
    CPUID_FEAT_EDX_SSE          = 1 << 25, // SSE instructions.
    CPUID_FEAT_EDX_SSE2         = 1 << 26, // SSE2 instructions.
    CPUID_FEAT_EDX_SS           = 1 << 27, // CPU cache supports self-snoop.
    CPUID_FEAT_EDX_HTT          = 1 << 28, // Hyper-threading.
    CPUID_FEAT_EDX_TM1          = 1 << 29, // Thermal monitor automatically limits temperature.
    CPUID_FEAT_EDX_IA64         = 1 << 30, // IA64 processor emulating x86.
    CPUID_FEAT_EDX_PBE          = 1 << 31, // Pending Break Enable (PBE# pin) wakeup support.

    CPUID_FEAT_ECX_SSE3         = 1 << 0,  // SSE3 instructions.
    CPUID_FEAT_ECX_PCLMUL       = 1 << 1,  // PCLMULQDQ support.
    CPUID_FEAT_ECX_DTES64       = 1 << 2,  // 64-bit debug store.
    CPUID_FEAT_ECX_MONITOR      = 1 << 3,  // MONITOR and MWAIT instructions.
    CPUID_FEAT_ECX_DS_CPL       = 1 << 4,  // CPL qualified debug store.
    CPUID_FEAT_ECX_VMX          = 1 << 5,  // Virtual Machine eXtensions.
    CPUID_FEAT_ECX_SMX          = 1 << 6,  // Safer Mode Extensions.
    CPUID_FEAT_ECX_EST          = 1 << 7,  // Enhanced SpeedStep.
    CPUID_FEAT_ECX_TM2          = 1 << 8,  // Thermal Monitor 2.
    CPUID_FEAT_ECX_SSSE3        = 1 << 9,  // Supplemental SSE3 instructions.
    CPUID_FEAT_ECX_CID          = 1 << 10, // L1 Context ID.
    CPUID_FEAT_ECX_SDBG         = 1 << 11, // Silicon Debug interface.
    CPUID_FEAT_ECX_FMA          = 1 << 12, // Fused multiply-add.
    CPUID_FEAT_ECX_CX16         = 1 << 13, // CMPXCHG16B instruction.
    CPUID_FEAT_ECX_ETPRD        = 1 << 14, // Can disable sending task priority messages.
    CPUID_FEAT_ECX_PDCM         = 1 << 15, // Perfmon & debug capability.
    CPUID_FEAT_ECX_PCIDE        = 1 << 17, // Process context identifiers.
    CPUID_FEAT_ECX_DCA          = 1 << 18, // Direct cache access for DMA writes.
    CPUID_FEAT_ECX_SSE4_1       = 1 << 19, // SSE4.1 instructions.
    CPUID_FEAT_ECX_SSE4_2       = 1 << 20, // SSE4.2 instructions.
    CPUID_FEAT_ECX_x2APIC       = 1 << 21, // x2APIC support.
    CPUID_FEAT_ECX_MOVBE        = 1 << 22, // MOVBE instruction (big-endian).
    CPUID_FEAT_ECX_POPCNT       = 1 << 23, // POPCNT instruction.
    CPUID_FEAT_ECX_TSC_DEAD     = 1 << 24, // APIC supports one-shot operation using a TSC deadline value.
    CPUID_FEAT_ECX_AES          = 1 << 25, // AES instruction set.
    CPUID_FEAT_ECX_XSAVE        = 1 << 26, // XSAVE, XRESTOR, XSETBV, XGETBV.
    CPUID_FEAT_ECX_OSXSAVE      = 1 << 27, // XSAVE enabled by OS.
    CPUID_FEAT_ECX_AVX          = 1 << 28, // Advanced Vector Extensions.
    CPUID_FEAT_ECX_F16C         = 1 << 29, // F16C (half-precision) FP support.
    CPUID_FEAT_ECX_RDRAND       = 1 << 30, // RDRAND (on-chip random number generator) support.
    CPUID_FEAT_ECX_HYPERVISOR   = 1 << 31, // Running on a hypervisor (always 0 on a real CPU, but also with some hypervisors).


    // Extended features.
    CPUID_FEAT_EBX_FSGSBASE     = 1 << 0,  // Access to base of %fs and %gs.
    CPUID_FEAT_EBX_TSC_ADJUST   = 1 << 1,  // IA32_TSC_ADJUST.
    CPUID_FEAT_EBX_SGX          = 1 << 2,  // Software Guard Extensions.
    CPUID_FEAT_EBX_BMI1         = 1 << 3,  // Bit Manipulation Instruction Set 1.
    CPUID_FEAT_EBX_HLE          = 1 << 4,  // Transactional Synchronization Extensions.
    CPUID_FEAT_EBX_AVX2         = 1 << 5,  // Advanced Vector Extensions 2.
    CPUID_FEAT_EBX_SMEP         = 1 << 7,  // Supervisor-Mode Execution Prevention.
    CPUID_FEAT_EBX_BMI2         = 1 << 8,  // Bit Manipulation Instruction Set 2.
    CPUID_FEAT_EBX_ERMS         = 1 << 9,  // Enhanced REP MOVSB/STOSB.
    CPUID_FEAT_EBX_INVPCID      = 1 << 10, // INVPCID instruction.
    CPUID_FEAT_EBX_RTM          = 1 << 11, // Transactional Synchronization Extensions.
    CPUID_FEAT_EBX_PQM          = 1 << 12, // Platform Quality of Service Monitoring.
    CPUID_FEAT_EBX_MPX          = 1 << 14, // Intel MPX (Memory Protection Extensions).
    CPUID_FEAT_EBX_PQE          = 1 << 15, // Platform Quality of Service Enforcement.
    CPUID_FEAT_EBX_AVX512       = 1 << 16, // AVX-512 Foundation.
    CPUID_FEAT_EBX_AVX512DQ     = 1 << 17, // AVX-512 Doubleword and Quadword Instructions.
    CPUID_FEAT_EBX_RDSEED       = 1 << 18, // RDSEED instruction.
    CPUID_FEAT_EBX_ADX          = 1 << 19, // Intel ADX (Multi-Precision Add-Carry Instruction Extensions).
    CPUID_FEAT_EBX_SMAP         = 1 << 20, // Supervisor Mode Access Prevention.
    CPUID_FEAT_EBX_AVX512IFMA   = 1 << 21, // AVX-512 Integer Fused Multiply-Add Instructions.
    CPUID_FEAT_EBX_PCOMMIT      = 1 << 22, // PCOMMIT instruction.
    CPUID_FEAT_EBX_CLFLUSHOPT   = 1 << 23, // CLFLUSHOPT instruction.
    CPUID_FEAT_EBX_CLWB         = 1 << 24, // CLWB instruction.
    CPUID_FEAT_EBX_INTELPT      = 1 << 25, // Intel Processor Trace.
    CPUID_FEAT_EBX_AVX512PF     = 1 << 26, // AVX-512 Prefetch Instructions.
    CPUID_FEAT_EBX_AVX512ER     = 1 << 27, // AVX-512 Exponential and Reciprocal Instructions.
    CPUID_FEAT_EBX_AVX512CD     = 1 << 28, // AVX-512 Conflict Detection Instructions.
    CPUID_FEAT_EBX_SHA          = 1 << 29, // Intel SHA extensions.
    CPUID_FEAT_EBX_AVX512BW     = 1 << 30, // AVX-512 Byte and Word Instructions.
    CPUID_FEAT_EBX_AVX512VL     = 1 << 31, // AVX-512 Vector Length Extensions.

    CPUID_FEAT_ECX_PREFETCHWT1  = 1 << 0,  // PREFETCHWT1 instruction.
    CPUID_FEAT_ECX_AVX512VBMI   = 1 << 1,  // AVX-512 Vector Bit Manipulation Instructions.
    CPUID_FEAT_ECX_UMIP         = 1 << 2,  // User-mode Instruction Prevention.
    CPUID_FEAT_ECX_PKU          = 1 << 3,  // Memory Protection Keys for User-mode pages.
    CPUID_FEAT_ECX_OSPKE        = 1 << 4,  // PKU enabled by OS.
    CPUID_FEAT_ECX_AVX512VBMI2  = 1 << 6,  // AVX-512 Vector Bit Manipulation Instructions 2.
    CPUID_FEAT_ECX_GFNI         = 1 << 8,  // Galois Field instructions.
    CPUID_FEAT_ECX_VAES         = 1 << 9,  // AES instruction set (VEX-256/EVEX).
    CPUID_FEAT_ECX_VPCLMULQDQ   = 1 << 10, // CLMUL instruction set (VEX-256/EVEX).
    CPUID_FEAT_ECX_AVX512VNNI   = 1 << 11, // AVX-512 Vector Neural Network Instructions.
    CPUID_FEAT_ECX_AVX512BITALG = 1 << 12, // AVX-512 BITALG instructions.
    CPUID_FEAT_ECX_AVX512VPOPCNT= 1 << 14, // AVX-512 Vector Population Count D/Q.
    CPUID_FEAT_ECX_MAWAU        = 1 << 17, // MPX Address-Width Adjust.
    CPUID_FEAT_ECX_RDPID        = 1 << 22, // Read Processor ID.
    CPUID_FEAT_ECX_SGX_LC       = 1 << 30, // SGX Launch Configuration.
 
    CPUID_FEAT_EDX_AVX512_4VNNIW= 1 << 2,  // AVX-512 4-register Neural Network Instructions.
    CPUID_FEAT_EDX_AVX512_4FMAPS= 1 << 3,  // AVX-512 4-register Multiply Accumulation Single precision.
    CPUID_FEAT_EDX_SPEC_CTRL    = 1 << 26, // Speculation Control.


    // Additional extended (AMD) features.
    CPUID_FEAT_EDX_SYSCALL      = 1 << 11, // SYSCALL and SYSRET instructions.
    CPUID_FEAT_EDX_MP           = 1 << 19, // Multiprocessor Capable.
    CPUID_FEAT_EDX_NX           = 1 << 20, // NX bit.
    CPUID_FEAT_EDX_MMXEXT       = 1 << 22, // Extended MMX.
    CPUID_FEAT_EDX_FXSR_OPT     = 1 << 25, // FXSAVE/FXRSTOR optimizations.
    CPUID_FEAT_EDX_PDPE1GB      = 1 << 26, // Gibibyte pages.
    CPUID_FEAT_EDX_RDTSCP       = 1 << 27, // RDTSCP instruction.
    CPUID_FEAT_EDX_LM           = 1 << 29, // Long mode.
    CPUID_FEAT_EDX_3DNOWEXT     = 1 << 30, // Extended 3DNow!
    CPUID_FEAT_EDX_3DNOW        = 1 << 31, // 3DNow!

    CPUID_FEAT_ECX_LAHF_LM      = 1 << 0,  // LAHF/SAHF in long mode.
    CPUID_FEAT_ECX_CMP_LEGACY   = 1 << 1,  // Hyperthreading not valid.
    CPUID_FEAT_ECX_SVM          = 1 << 2,  // Secure Virtual Machine.
    CPUID_FEAT_ECX_EXTAPIC      = 1 << 3,  // Extended APIC space.
    CPUID_FEAT_ECX_CR8_LEGACY   = 1 << 4,  // CR8 in 32-bit mode.
    CPUID_FEAT_ECX_ABM          = 1 << 5,  // Advanced bit manipulation (lzcnt and popcnt).
    CPUID_FEAT_ECX_SSE4A        = 1 << 6,  // SSE4a.
    CPUID_FEAT_ECX_MISALIGNSSE  = 1 << 7,  // Misaligned SSE mode.
    CPUID_FEAT_ECX_3DNOWPREFETCH= 1 << 8,  // PREFETCH and PREFETCHW instructions.
    CPUID_FEAT_ECX_OSVW         = 1 << 9,  // OS Visible Workaround.
    CPUID_FEAT_ECX_IBS          = 1 << 10, // Instruction Based Sampling.
    CPUID_FEAT_ECX_XOP          = 1 << 11, // XOP instruction set.
    CPUID_FEAT_ECX_SKINIT       = 1 << 12, // SKINIT/STGI instructions.
    CPUID_FEAT_ECX_WDT          = 1 << 13, // Watchdog timer.
    CPUID_FEAT_ECX_LWP          = 1 << 15, // Light Weight Profiling.
    CPUID_FEAT_ECX_FMA4         = 1 << 16, // 4 operands fused multiply-add.
    CPUID_FEAT_ECX_TCE          = 1 << 17, // Translation Cache Extension.
    CPUID_FEAT_ECX_NODEID_MSR   = 1 << 19, // NodeID MSR.
    CPUID_FEAT_ECX_TBM          = 1 << 21, // Trailing Bit Manipulation.
    CPUID_FEAT_ECX_TOPOEXT      = 1 << 22, // Topology Extensions.
    CPUID_FEAT_ECX_PERFCTR_CORE = 1 << 23, // Core performance counter extensions.
    CPUID_FEAT_ECX_PERFCTR_NB   = 1 << 24, // NB performance counter extensions.
    CPUID_FEAT_ECX_BPEXT        = 1 << 26, // Data breakpoint extensions.
    CPUID_FEAT_ECX_PTSC         = 1 << 27, // Performance TSC.
    CPUID_FEAT_ECX_PERFCTR_L2   = 1 << 28, // L2I perf counter extensions.
    CPUID_FEAT_ECX_MWAITX       = 1 << 29  // MWAIT extensions.
};

extern bool cpuid_query(uint32_t function, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
extern void cpuid_print_capabilities();

#endif
