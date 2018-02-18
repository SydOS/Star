typedef struct {
	uint8_t status;
	uint32_t size;
} alloc_t;

extern char* malloc(size_t size);
extern void memory_init(multiboot_info_t* mboot_info, uint32_t kernel_end);
extern void memory_print_out();
extern void free(void *mem);
extern void pfree(void *mem);
extern char* pmalloc(size_t size);
extern void* memset16 (void *ptr, uint16_t value, size_t num);