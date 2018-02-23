#include <main.h>
#include <multiboot.h>

typedef struct {
	uint8_t status;
	uint32_t size;
} alloc_t;




extern char* malloc(size_t size);
extern void memory_init();
extern void memory_print_out();
extern void free(void *mem);
extern void pfree(void *mem);
extern char* pmalloc(size_t size);
extern void* memset16 (void *ptr, uint16_t value, size_t num);