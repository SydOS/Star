#include <main.h>
#include <multiboot.h>

typedef struct {
	uint8_t status;
	uint32_t size;
} alloc_t;


struct mem_info {
	multiboot_info_t *mbootInfo;
	multiboot_memory_map_t *mmap;
	uint32_t mmapLength;

	uint32_t kernelStart;
	uint32_t kernelEnd;
	uint32_t mbootStart;
	uint32_t mbootEnd;

	uint32_t kernelHeapStart;
	uint32_t kernelHeapPosition;

	uint32_t isaDmaStart;
	uint32_t isaDmaEnd;

	uint32_t memLower;
	uint32_t memUpper;
	uint32_t highestFreeAddress;
};
typedef struct mem_info mem_info_t;

extern char* malloc(size_t size);
extern void memory_init(multiboot_info_t* mboot_info);
extern void memory_print_out();
extern void free(void *mem);
extern void pfree(void *mem);
extern char* pmalloc(size_t size);
extern void* memset16 (void *ptr, uint16_t value, size_t num);