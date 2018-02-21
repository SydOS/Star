#ifndef STRING_H
#define STRING_H

extern int32_t memcmp(const void *str1, const void *str2, size_t n);
extern void memcpy(uint8_t *src, uint8_t *dest, size_t n);
extern void* memmove(void *str1, const void *str2, size_t n);
extern void* memset(void *str, int32_t c, size_t n);
extern char* strcat(char *dest, const char *src);
extern int32_t strcmp(const char *str1, const char *str2);
extern char* strcpy(char *dest, const char *src);
extern char* strncpy(char *dest, const char *src, size_t n);
extern size_t strlen(const char* str);

#endif
