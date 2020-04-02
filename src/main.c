/*
 * File: main.c
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
#include <io.h>
#include <string.h>
#include <kprint.h>
#include <kernel/gdt.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/acpi/acpi.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/kheap.h>
#include <kernel/tasking.h>
#include <kernel/timer.h>
#include <kernel/interrupts/smp.h>
#include <kernel/cpuid.h>
#include <driver/vga.h>
#include <driver/storage/floppy.h>
#include <driver/serial.h>
#include <driver/pci.h>
#include <driver/speaker.h>
#include <driver/ps2/ps2.h>
#include <libs/keyboard.h>
#include <driver/rtc.h>
#include <kernel/multitasking/syscalls.h>

#include <driver/usb/devices/usb_device.h>

#include <kernel/networking/networking.h>
#include <kernel/networking/protocols/arp.h>

#include <acpi.h>

#include <kernel/vfs/vfs.h>

#include <driver/fs/fat.h>
#include <kernel/storage/storage.h>

// Displays a kernel panic message and halts the system.
void panic(const char *format, ...) {
	// Disable interrupts.
	interrupts_disable();

    // Get args.
    va_list args;
	va_start(args, format);

	// Show panic.
	kprintf_nolock("\e[41m\nPANIC:\n");
	kprintf_va(false, format, args);
	kprintf_nolock("\n\nHalted.");

	// Halt other processors.
	//lapic_send_nmi_all();

	// Halt forever.
	asm volatile ("hlt");
	while (true);
}

/**
 * The main function for the kernel, called from boot.asm
 */
void kernel_main() {
	// Initialize serial for logging.
	serial_init();

	// Initialize VGA.
	vga_init();

	// Initialize the GDT.
	gdt_init_bsp();

	// Initialize memory system.
	pmm_init();
    paging_init();
	kheap_init();

	// Initialize ACPI and interrupts.
	acpi_init();
	interrupts_init_bsp();

	// Initialize timer.
    timer_init();

	kprintf("Initializing PS/2...\n");
	ps2_init();

	// Initialize SMP.
	smp_init();

	// Print CPUID info.
	//cpuid_print_capabilities();

	// Start up tasking and create kernel task.
	//kprintf("Starting tasking...\n");
	tasking_init();

	// We should never get here.
	panic("MAIN: Tasking failed to start!\n");
}

void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		kprintf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			kprintf(" ");
			if ((i+1) % 16 == 0) {
				kprintf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					kprintf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					kprintf("   ");
				}
				kprintf("|  %s \n", ascii);
			}
		}
	}
}

void kernel_late() {
	//kprintf("MAIN: Adding second kernel thread...\n");
	//tasking_thread_add_kernel(tasking_thread_create("hmm", (uintptr_t)hmmm_thread, 12, 3, 4));

	kprintf("MAIN: Adding second process...\n"); 
	//tasking_process_add(tasking_process_create("another one", tasking_thread_create("ring3", (uintptr_t)secondprocess_thread, 0, 0, 0), false));

	acpi_late_init();
	
	// Initialize floppy.
	floppy_init();

	//pci_init();

	// Initialize VFS.
	vfs_init();

	// Print info.
	kprintf("\e[92mKernel is located at 0x%p!\n", memInfo.kernelStart);
	kprintf("Detected usable RAM: %uMB\n", memInfo.memoryKb / 1024);
	if (memInfo.paeEnabled)
		kprintf("PAE enabled!\n");
	if (memInfo.nxEnabled)
		kprintf("NX enabled!\n");
	kprintf("\e[0m");
	
	//rtc_init();
	//kprintf("24 hour time: %d, binary input: %d\n", rtc_settings->twentyfour_hour_time, rtc_settings->binary_input);
	//kprintf("%d:%d:%d %d/%d/%d\n", rtc_time->hours, rtc_time->minutes, rtc_time->seconds, rtc_time->month, rtc_time->day, rtc_time->year);

	// Initialize networking.
	//networking_init();

	// Print logo.
	kprintf("\n\e[94m");
	kprintf("   _____           _  ____   _____ \n");
	kprintf("  / ____|         | |/ __ \\ / ____|\n");
	kprintf(" | (___  _   _  __| | |  | | (___  \n");
	kprintf("  \\___ \\| | | |/ _` | |  | |\\___ \\ \n");
	kprintf("  ____) | |_| | (_| | |__| |____) |\n");
	kprintf(" |_____/ \\__, |\\__,_|\\____/|_____/ \n");
	kprintf("          __/ |                    \n");
	kprintf("         |___/                     \n");
	kprintf("\e[36mCopyright (c) Sydney Erickson, John Davis 2017 - 2020\e[0m\n\n");

    // Ring serial terminals.
	kprintf("\a");

	// Create userspace process.
    kprintf("Creating userspace process...\n");
	int32_t rootDirHandle = vfs_open("/INIT", 0);
	if (rootDirHandle == -1)
		panic("Could not get handler for /init.");

    uint8_t *progbuffer = (uint8_t*)kheap_alloc(512);
	if (progbuffer == NULL)
		panic("Could not alloc memory space for /init.");
    int32_t result = vfs_read(rootDirHandle, progbuffer, 512);
	vfs_close(rootDirHandle);

	DumpHex(progbuffer, 512);

	kprintf("Main thread jumping into init program...\n");
	void (*foo)(void) = &progbuffer[0];
	foo();

	// TODO: Get this loaded into user space memory and jump to it in user space
	//process_t *initProcess = tasking_process_create(kernelProcess, "init", true, "init_main", foo, 0, 0, 0);
    //tasking_thread_schedule_proc(initProcess->MainThread, 0);

    kheap_free(progbuffer);
	panic("Init program returned.");
}
