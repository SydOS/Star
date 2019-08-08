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
	kprintf_nolock("\nPANIC:\n");
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
	cpuid_print_capabilities();

	// Start up tasking and create kernel task.
	kprintf("Starting tasking...\n");
	tasking_init();

	// We should never get here.
	panic("MAIN: Tasking failed to start!\n");
}

#define OCTETS 4
#define DELIMINATOR '.'
#define DIGIT_OFFSET 48

void hmmm_thread(uintptr_t arg1, uintptr_t arg2) {
	while (1) { 
		kprintf("hmm(): %u seconds\n", timer_ticks() / 1000);
		sleep(2000);
	 }
}

void secondprocess_thread(void) {
	while (1) { 
		uint64_t uptime = 0;
		syscalls_syscall(&uptime, 0, 0, 0, 0, 0, SYSCALL_UPTIME);
		syscalls_kprintf("Hi from ring 3, via syscall! Uptime: %u\n", uptime);
		sleep(4000);
	 }
}

static void print_usb_children(usb_device_t *usbDevice, uint8_t level) {
	// If there are no children, just return.
	if (usbDevice->Children == NULL)
		return;

	usb_device_t *childDevice = usbDevice->Children;
	while (childDevice != NULL) {
		kprintf(" ");
		for (uint8_t i = 0; i < level; i++)
			kprintf("-");
		kprintf(" ");
		kprintf("%4X:%4X %s %s\n", childDevice->VendorId, childDevice->ProductId, childDevice->VendorString, childDevice->ProductString);
		
		// Iterate through children.
		print_usb_children(childDevice, level + 1);

		// Move to next device.
		childDevice = childDevice->Next;
	}
}

// USERSPACE THREAD.
static void kernel_init_thread(void) {
    // Open file.
    int32_t handle = (int32_t)syscalls_syscall("/HELLO", 0, 0, 0, 0, 0, SYSCALL_OPEN);

	// Print ticks.
    syscalls_kprintf("TASKING: Test from ring 3: %u ticks\n", timer_ticks());
    sleep(1000);
    syscalls_kprintf("TASKING: opening file\n");

	// Seek to byte 0 and read 368 bytes of test program
	uint8_t data[368];
	syscalls_syscall(handle, 0, 0, 0, 0, 0, SYSCALL_SEEK);
	syscalls_syscall(handle, data, 368, 0, 0, 0, SYSCALL_READ);
	syscalls_kprintf("TASKING: read file\n");

	for (int i = 0; i < 368; i++) {
		syscalls_kprintf("%x ", data[i]);
	}
	syscalls_kprintf("\n");

	syscalls_kprintf("%p\n", &data);
	syscalls_kprintf("%s\n", *&data);

	uint8_t *p = data;
	for (int i = 0; i < 368; i++) {
		syscalls_kprintf("%x ", *(p + i));
	}
	syscalls_kprintf("\n");
	syscalls_kprintf("%p\n", &p);
	syscalls_kprintf("%s\n", *&p);
	p = p + 241;
	syscalls_kprintf("entry: %x\n", *p);
	syscalls_kprintf("TASKING: new p %p\n", p);

	void (*foo)(void) = p;
	syscalls_kprintf("%p\n", p);
	foo();
}

void kernel_late() {
	//kprintf("MAIN: Adding second kernel thread...\n");
	//tasking_thread_add_kernel(tasking_thread_create("hmm", (uintptr_t)hmmm_thread, 12, 3, 4));

	kprintf("MAIN: Adding second process...\n"); 
	//tasking_process_add(tasking_process_create("another one", tasking_thread_create("ring3", (uintptr_t)secondprocess_thread, 0, 0, 0), false));

	acpi_late_init();
	
	// Initialize floppy.
	floppy_init();

	pci_init();

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
	
	rtc_init();
	kprintf("24 hour time: %d, binary input: %d\n", rtc_settings->twentyfour_hour_time, rtc_settings->binary_input);
	kprintf("%d:%d:%d %d/%d/%d\n", rtc_time->hours, rtc_time->minutes, rtc_time->seconds, rtc_time->month, rtc_time->day, rtc_time->year);

	// Initialize networking.
	networking_init();

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
	kprintf("\e[36mCopyright (c) Sydney Erickson, John Davis 2017 - 2018\e[0m\n\n");

    // Ring serial terminals.
	kprintf("\a");

	// Create userspace process.
    kprintf("Creating userspace process...\n");
    process_t *initProcess = tasking_process_create(kernelProcess, "init", true, "init_main", kernel_init_thread, 0, 0, 0);
    tasking_thread_schedule_proc(initProcess->MainThread, 0);

	char buffer[100];
	while (true) {
		kprintf("\e[96mroot@sydos ~:\e[0m ");

		uint16_t i = 0;
		while (i < 98) {
			uint16_t k = keyboard_get_last_key();
			while (k == KEYBOARD_KEY_UNKNOWN && serial_received() == 0)
				k = keyboard_get_last_key();

			if (serial_received() != 0) {
				char c = serial_read();
				if (c == '\r' || c == '\n')
					break;
				else if (c == '\b' || c == 127) {
					if (i > 0) {
						i--;
						kprintf("\b \b");
					}
				}
				else {
					kprintf("%c", c);
					buffer[i++] = c;
				}
			}
			else {
				if (k == KEYBOARD_KEY_ENTER)
					break;
				else if (k == KEYBOARD_KEY_BACKSPACE) {
					if (i > 0) {
						i--;
						kprintf("\b \b");
					}
				}
				else {
					char c = keyboard_get_ascii(k);
					if (c) {
						kprintf("%c", c);
						buffer[i++] = c;
					}
				}
			}		
		}

		// Terminate.
		buffer[i] = '\0';
		kprintf("\n");


		if (strcmp(buffer, "exit") == 0)
			ps2_reset_system();

		else if (strcmp(buffer, "uptime") == 0)
			kprintf("Current uptime: %i milliseconds.\n", timer_ticks());
		else if (strcmp(buffer, "floppy") == 0) {
				// Mount? floppy drive. No partitions here.
			//fat_init(storageDevices, PARTITION_NONE);
		}

		else if (strcmp(buffer, "corp") == 0)
			kprintf("Hacking CorpNewt's computer and installing SydOS.....\n");
		else if (strncmp(buffer, "beep ", 4) == 0) {
			// Get hertz.
			char* hzStr = buffer + 5;
			char* msStr = hzStr;
			uint32_t hzLen = strlen(hzStr);
			for (uint16_t i = 0; i < hzLen; i++) {
				if (hzStr[i] == ' ') {
					hzLen = i;
					msStr = hzStr + hzLen + 1;
					break;
				}
			}

			// Get freq.
			char *hzStrTmp = (char*)kheap_alloc(hzLen + 1);
			strncpy(hzStrTmp, hzStr, hzLen);
			hzStrTmp[hzLen] = '\0';
			uint32_t hz = atoi(hzStrTmp);
			kheap_free(hzStrTmp);

			// Get duration.
			uint32_t ms = atoi(msStr);

			kprintf("Beeping at %u hertz for %u ms...\n", hz, ms);
			speaker_play_tone(hz, ms);
		}
		else if (strncmp(buffer, "arpping ", 7) == 0) {
			char* ipstr = buffer + 8;
			
			// Parse IPv4 string
			uint8_t octets[4];
			int array_len = strlen(ipstr);
			int multiplier = 1;
			int current_num = 0;
			int octet_index = OCTETS - 1;
			for (int index = array_len - 1; index >= 0 && octet_index >= 0; index--) {
				if (ipstr[index] == DELIMINATOR) {
					octets[octet_index] = current_num;
					octet_index--;

					multiplier = 1;
					current_num = 0;
				} else if (ipstr[index] >= DIGIT_OFFSET && ipstr[index] <= DIGIT_OFFSET+10) {
					current_num += (ipstr[index] - DIGIT_OFFSET) * multiplier;
					multiplier *= 10;
				}
			}
			octets[octet_index] = current_num;

			// ARP request it
			arp_frame_t* frame = arp_get_mac_address(&NetDevices[0], octets);
			kheap_free(frame);
		}
		else if (strcmp(buffer, "lsusb") == 0) {
			usb_device_t *usbDevice = StartUsbDevice;
			while (usbDevice != NULL) {
				kprintf("%4X:%4X %s %s\n", usbDevice->VendorId, usbDevice->ProductId, usbDevice->VendorString, usbDevice->ProductString);
				
				// Iterate through children.
				print_usb_children(usbDevice, 1);

				// Move to next device.
				usbDevice = usbDevice->Next;
			}
		}
		else if (strcmp(buffer, "lsnet") == 0) {
			networking_print_devices();
		}
		else if (strcmp(buffer, "free") == 0) {
			kprintf("Free page count: %u\n", pmm_frames_available_long());
		}
	}
}
