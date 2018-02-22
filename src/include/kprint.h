#ifndef KPRINT_H
#define KPRINT_H

#include <stdarg.h>

extern void kputchar(char c);
extern void kputchar_hex(uint8_t num, bool capital, bool pad);
extern void kputstring(const char *str);
extern void kprint_int(int64_t num);
extern void kprint_uint(uint64_t num);
extern void kprint_hex(uint64_t num, bool capital, bool pad);
extern void kprintf(const char* format, ...);

#endif