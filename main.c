#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>

#include "main.h"

void kernel_main(void) {
	vga_initialize();
	vga_writes("SydOS Pre-Alpha\n");
}