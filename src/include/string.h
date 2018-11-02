#ifndef STRING_H
#define STRING_H

extern int32_t memcmp(const void *str1, const void *str2, size_t n);
extern void memcpy(uint8_t *dest, uint8_t *src, size_t n);
extern void* memmove(void *str1, const void *str2, size_t n);
extern void* memset(void *str, int32_t c, size_t n);
extern void* memset16 (void *ptr, uint16_t value, size_t num);
extern char* strcat(char *dest, const char *src);
extern char* strncat(char *dest, const char *src, size_t n);
extern int32_t strcmp(const char *str1, const char *str2);
extern int32_t strncmp(const char *str1, const char *str2, size_t n);
extern char* strcpy(char *dest, const char *src);
extern char* strncpy(char *dest, const char *src, size_t n);
extern size_t strlen(const char* str);

extern char *strchr(const char *str, int32_t c);
extern char *strrchr(const char *str, int32_t c);

extern int32_t toupper(int32_t c);
extern int32_t tolower(int32_t c);
extern int32_t isdigit(int32_t c);
extern int32_t isxdigit(int32_t c);
extern int32_t isspace(int32_t c);
extern int32_t isprint(int32_t c);

#endif
