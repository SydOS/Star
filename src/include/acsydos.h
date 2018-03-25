

#ifndef __ACSYDOS_H__
#define __ACSYDOS_H__


/* Common (in-kernel/user-space) ACPICA configuration */

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0
#define ACPI_IGNORE_PACKAGE_RESOLUTION_ERRORS

#define ACPI_CACHE_T                ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE        1


#include <main.h>
#include <string.h>

#ifdef __KERNEL__

//#define ACPI_USE_SYSTEM_INTTYPES
#define ACPI_USE_GPE_POLLING

/* Kernel specific ACPICA configuration */

#ifdef CONFIG_ACPI_REDUCED_HARDWARE_ONLY
#define ACPI_REDUCED_HARDWARE 1
#endif

#ifdef CONFIG_ACPI_DEBUGGER
#define ACPI_DEBUGGER
#endif

#ifdef CONFIG_ACPI_DEBUG
#define ACPI_MUTEX_DEBUG
#endif

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/spinlock_types.h>
#ifdef EXPORT_ACPI_INTERFACES
#include <linux/export.h>
#endif
#ifdef CONFIG_ACPI
#include <asm/acenv.h>
#endif

#define ACPI_INIT_FUNCTION __init

#ifndef CONFIG_ACPI

/* External globals for __KERNEL__, stubs is needed */

#define ACPI_GLOBAL(t,a)
#define ACPI_INIT_GLOBAL(t,a,b)

/* Generating stubs for configurable ACPICA macros */

#define ACPI_NO_MEM_ALLOCATIONS

/* Generating stubs for configurable ACPICA functions */

#define ACPI_NO_ERROR_MESSAGES
#undef ACPI_DEBUG_OUTPUT

/* External interface for __KERNEL__, stub is needed */

#define ACPI_EXTERNAL_RETURN_STATUS(Prototype) \
    static ACPI_INLINE Prototype {return(AE_NOT_CONFIGURED);}
#define ACPI_EXTERNAL_RETURN_OK(Prototype) \
    static ACPI_INLINE Prototype {return(AE_OK);}
#define ACPI_EXTERNAL_RETURN_VOID(Prototype) \
    static ACPI_INLINE Prototype {return;}
#define ACPI_EXTERNAL_RETURN_UINT32(Prototype) \
    static ACPI_INLINE Prototype {return(0);}
#define ACPI_EXTERNAL_RETURN_PTR(Prototype) \
    static ACPI_INLINE Prototype {return(NULL);}

#endif /* CONFIG_ACPI */

/* Host-dependent types and defines for in-kernel ACPICA */

#define ACPI_MACHINE_WIDTH          BITS_PER_LONG
#define ACPI_USE_NATIVE_MATH64
#define ACPI_EXPORT_SYMBOL(symbol)  EXPORT_SYMBOL(symbol);
#define strtoul                     simple_strtoul

#define ACPI_CACHE_T                struct kmem_cache
#define ACPI_SPINLOCK               spinlock_t *
#define ACPI_CPU_FLAGS              unsigned long

/* Use native linux version of AcpiOsAllocateZeroed */

#define USE_NATIVE_ALLOCATE_ZEROED

/*
 * Overrides for in-kernel ACPICA
 */
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInitialize
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTerminate
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAllocate
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAllocateZeroed
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsFree
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireObject
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetThreadId
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateLock

/*
 * OSL interfaces used by debugger/disassembler
 */
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadable
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritable
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInitializeDebugger
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTerminateDebugger

/*
 * OSL interfaces used by utilities
 */
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsRedirectOutput
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByName
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByIndex
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByAddress
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsOpenDirectory
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetNextFilename
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCloseDirectory

#define ACPI_MSG_ERROR          KERN_ERR "ACPI Error: "
#define ACPI_MSG_EXCEPTION      KERN_ERR "ACPI Exception: "
#define ACPI_MSG_WARNING        KERN_WARNING "ACPI Warning: "
#define ACPI_MSG_INFO           KERN_INFO "ACPI: "

#define ACPI_MSG_BIOS_ERROR     KERN_ERR "ACPI BIOS Error (bug): "
#define ACPI_MSG_BIOS_WARNING   KERN_WARNING "ACPI BIOS Warning (bug): "

/*
 * Linux wants to use designated initializers for function pointer structs.
 */
#define ACPI_STRUCT_INIT(field, value)  .field = value

#else /* !__KERNEL__ */

//#define ACPI_USE_STANDARD_HEADERS

#ifdef ACPI_USE_STANDARD_HEADERS
//#include <unistd.h>
#endif

/* Define/disable kernel-specific declarators */

#ifndef __init
#define __init
#endif
#ifndef __iomem
#define __iomem
#endif

/* Host-dependent types and defines for user-space ACPICA */

#define ACPI_FLUSH_CPU_CACHE()
#define ACPI_CAST_PTHREAD_T(Pthread) ((ACPI_THREAD_ID) (Pthread))

#if defined(__ia64__)    || (defined(__x86_64__) && !defined(__ILP32__)) ||\
    defined(__aarch64__) || defined(__PPC64__) ||\
    defined(__s390x__)
#define ACPI_MACHINE_WIDTH          64
#define COMPILER_DEPENDENT_INT64    long
#define COMPILER_DEPENDENT_UINT64   unsigned long
#else
#define ACPI_MACHINE_WIDTH          32
#define COMPILER_DEPENDENT_INT64    long long
#define COMPILER_DEPENDENT_UINT64   unsigned long long
#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64
#endif

#ifndef __cdecl
#define __cdecl
#endif

#endif /* __KERNEL__ */

#undef ACPI_USE_SYSTEM_INTTYPES

#endif /* __ACLINUX_H__ */
