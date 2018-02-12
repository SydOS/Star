#ifndef TOOLS_H
#define TOOLS_H

extern char* itoa(int value, char* result, int base);
extern char* utoa(uint32_t value, char* result, int base);
extern size_t strlen(const char* str);
extern void sleep(uint32_t ms);
extern uint32_t rand();
extern uint32_t maxrand(uint32_t seed, uint32_t max);

#endif