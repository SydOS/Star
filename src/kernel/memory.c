#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <multiboot.h>
#include <string.h>
#include <math.h>
#include <driver/vga.h>
#include <kernel/memory.h>

#define MAX_PAGE_ALIGNED_ALLOCS 32

uint32_t last_alloc = 0;
uint32_t heap_end = 0;
uint32_t heap_begin = 0;
uint32_t pheap_begin = 0;
uint32_t pheap_end = 0;
uint8_t *pheap_desc = 0;
uint32_t memory_used = 0;

extern uint32_t kernel_end;
/**
 * Kernel's starting address in RAM
 */
extern uint32_t kernel_base;

// Used to store info about memory in the system.
mem_info_t memInfo;

void memory_print_out()
{
	kprintf("Memory used: %u bytes\nMemory free: %u bytes\n", memory_used, heap_end - heap_begin - memory_used);
	
	kprintf("PHeap start: 0x%X\nPHeap end: 0x%X\n", pheap_begin, pheap_end);
}

void memory_init(multiboot_info_t* mbootInfo) {
	// Ensure a memory map is present.
	if ((mbootInfo->flags & MULTIBOOT_INFO_MEM_MAP) == 0)
	{
		kprintf("NO MULTIBOOT MEMORY INFO FOUND!\n");
		// Kernel should die at this point.....
		while (true);
	}

	// Store away Multiboot info.
	memInfo.mbootInfo = mbootInfo;
	memInfo.mmap = (multiboot_memory_map_t*)mbootInfo->mmap_addr;
	memInfo.mmapLength = mbootInfo->mmap_length;

	// Store where the Multiboot info structure actually resides in memory.
	memInfo.mbootStart = (uint32_t)mbootInfo;
	memInfo.mbootEnd = (uint32_t)(mbootInfo + sizeof(multiboot_info_t));
	memInfo.memLower = mbootInfo->mem_lower;
	memInfo.memUpper = mbootInfo->mem_upper;

	// Store where the kernel is. These come from the linker.
	memInfo.kernelStart = &kernel_base;
	memInfo.kernelEnd = &kernel_end;

	// Get the highest free address before the first hole.
	memInfo.highestFreeAddress = memInfo.memUpper * 1024; // memUpper is in KB, convert to bytes.

	// Start the kernel heap on the first aligned page directly after where the kernel is.
	memInfo.kernelHeapStart = (memInfo.kernelEnd + 0x1000) & 0xFFFFF000;
	memInfo.kernelHeapPosition = memInfo.kernelHeapStart;

	// Determine memory required and allocate for the bitset.
	uint32_t bitsetLength = memInfo.highestFreeAddress / 0x1000;
	uint32_t bitsetSize = DIVIDE_ROUND_UP(bitsetLength, 32) * sizeof(uint32_t);

	// Print summary.
	kprintf("Kernel start: 0x%X | Kernel end: 0x%X\n", memInfo.kernelStart, memInfo.kernelEnd);
	kprintf("Multiboot info start: 0x%X | Multiboot end: 0x%X\n", memInfo.mbootStart, memInfo.mbootEnd);
	kprintf("Heap start: 0x%X\n", memInfo.kernelHeapStart);

	//last_alloc = kernel_end + 0x1000;
	//heap_begin = last_alloc;
	//pheap_end = 0x400000;
	//pheap_begin = pheap_end - (MAX_PAGE_ALIGNED_ALLOCS * 4096);
	//heap_end = pheap_begin;
	//memset((char *)heap_begin, 0, heap_end - heap_begin);
	//pheap_desc = (uint8_t *)malloc(MAX_PAGE_ALIGNED_ALLOCS);

	kprintf("Physical memory map:\n");
		uint64_t memory = 0;

		uint32_t base = mbootInfo->mmap_addr;
		uint32_t end = base + mbootInfo->mmap_length;
		multiboot_memory_map_t* entry = (multiboot_memory_map_t*)base;

		for(; base < end; base += entry->size + sizeof(uint32_t))
		{
			entry = (multiboot_memory_map_t*)base;

			// Print out info.
			kprintf("region start: 0x%llX length: 0x%llX type: 0x%X\n", entry->addr, entry->len, (uint64_t)entry->type);

			if(entry->type == 1 && ((uint32_t)entry->addr) > 0)
				memory += entry->len;
		}

		memory = memory / 1024 / 1024;
		kprintf("Detected RAM: %uMB\n", memory);
		kprintf("Highest free address: 0x%X\n", memInfo.highestFreeAddress);

		
}


char* malloc(size_t size)
{
	if(!size) return 0;

	/* Loop through blocks and find a block sized the same or bigger */
	uint8_t *mem = (uint8_t *)heap_begin;
	while((uint32_t)mem < last_alloc)
	{
		alloc_t *a = (alloc_t *)mem;
		/* If the alloc has no size, we have reaced the end of allocation */
		//mprint("mem=0x%x a={.status=%d, .size=%d}\n", mem, a->status, a->size);
		if(!a->size)
			goto nalloc;
		/* If the alloc has a status of 1 (allocated), then add its size
		 * and the sizeof alloc_t to the memory and continue looking.
		 */
		if(a->status) {
			mem += a->size;
			mem += sizeof(alloc_t);
			mem += 4;
			continue;
		}
		/* If the is not allocated, and its size is bigger or equal to the
		 * requested size, then adjust its size, set status and return the location.
		 */
		if(a->size >= size)
		{
			/* Set to allocated */
			a->status = 1;

			//mprint("RE:Allocated %d bytes from 0x%x to 0x%x\n", size, mem + sizeof(alloc_t), mem + sizeof(alloc_t) + size);
			memset(mem + sizeof(alloc_t), 0, size);
			memory_used += size + sizeof(alloc_t);
			return (char *)(mem + sizeof(alloc_t));
		}
		/* If it isn't allocated, but the size is not good, then
		 * add its size and the sizeof alloc_t to the pointer and
		 * continue;
		 */
		mem += a->size;
		mem += sizeof(alloc_t);
		mem += 4;
	}

	nalloc:;
	if(last_alloc+size+sizeof(alloc_t) >= heap_end)
	{
		//set_task(0);
		//panic("Cannot allocate %d bytes! Out of memory.\n", size);
	}
	alloc_t *alloc = (alloc_t *)last_alloc;
	alloc->status = 1;
	alloc->size = size;

	last_alloc += size;
	last_alloc += sizeof(alloc_t);
	last_alloc += 4;
	//mprint("Allocated %d bytes from 0x%x to 0x%x\n", size, (uint32_t)alloc + sizeof(alloc_t), last_alloc);
	memory_used += size + 4 + sizeof(alloc_t);
	memset((char *)((uint32_t)alloc + sizeof(alloc_t)), 0, size);
	return (char *)((uint32_t)alloc + sizeof(alloc_t));
/*
	char* ret = (char*)last_alloc;
	last_alloc += size;
	if(last_alloc >= heap_end)
	{
		set_task(0);
		panic("Cannot allocate %d bytes! Out of memory.\n", size);
	}
	mprint("Allocated %d bytes from 0x%x to 0x%x\n", size, ret, last_alloc);
	return ret;*/
}

void free(void *mem)
{
	alloc_t *alloc = (mem - sizeof(alloc_t));
	memory_used -= alloc->size + sizeof(alloc_t);
	alloc->status = 0;
}

void pfree(void *mem)
{
	if(mem < pheap_begin || mem > pheap_end) return;
	/* Determine which page is it */
	uint32_t ad = (uint32_t)mem;
	ad -= pheap_begin;
	ad /= 4096;
	/* Now, ad has the id of the page */
	pheap_desc[ad] = 0;
	return;
}

char* pmalloc(size_t size)
{
	/* Loop through the avail_list */
	for(int i = 0; i < MAX_PAGE_ALIGNED_ALLOCS; i++)
	{
		if(pheap_desc[i]) continue;
		pheap_desc[i] = 1;
		return (char *)(pheap_begin + i*4096);
	}
	return 0;
}

void* memset16 (void *ptr, uint16_t value, size_t num)
{
	uint16_t* p = ptr;
	while(num--)
		*p++ = value;
	return ptr;
}