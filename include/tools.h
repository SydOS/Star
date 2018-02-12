#ifndef TOOLS_H
#define TOOLS_H

extern char* itoa(int value, char* result, int base);
extern char* utoa(uint32_t value, char* result, int base);
extern size_t strlen(const char* str);
void sleep(uint32_t ms);

#endif