#include <main.h>
#include <string.h>

// Compares the first n bytes of memory area str1 and memory area str2.
int32_t memcmp(const void *str1, const void *str2, size_t n) {
	const uint8_t *s1 = (const uint8_t*)str1;
	const uint8_t *s2 = (const uint8_t*)str2;

	// Compare bytes.
	for (size_t i = 0; i < n; i++)
	{
		if (s1[i] < s2[i])
			return -1;
		else if (s2[i] < s1[i])
			return 1;
	}
	return 0;
}

// Copies n bytes from memory area src to memory area dest.
void memcpy(uint8_t *dest, uint8_t *src, size_t n) {
	// Copy bytes.
	for (size_t i = 0; i < n; i++)
	{
		dest[i] = src[i];
		//kprintf("Byte %i\n", i);
	}
}

// Copies n bytes from memory area str1 to memory area str2,
// but for overlapping memory blocks, memmove() is a safer approach than memcpy().
void* memmove(void *str1, const void *str2, size_t n) {
	uint8_t *s1 = (uint8_t*)str1;
	const uint8_t *s2 = (const uint8_t*)str2;

	// Copy bytes.
	if (s1 < s2)
	{
		for (size_t i = 0; i < n; i++)
			s1[i] = s2[i];
	}
	else
	{
		for (size_t i = n; i != 0; i--)
			s1[i-1] = s2[i-1];
	}
	return str1;
}

// Copies the character c to the first n characters of the string pointed to by the argument str.
void* memset(void *str, int32_t c, size_t n) {
	uint8_t *s = (uint8_t*)str;

	// Copy byte.
	for (size_t i = 0; i < n; i++)
		s[i] = (uint8_t)c;
	return str;
}

void* memset16 (void *ptr, uint16_t value, size_t num) {
	uint16_t* p = ptr;
	while(num--)
		*p++ = value;
	return ptr;
}

// Appends the string pointed to by src to the end of the string pointed to by dest.
char* strcat(char *dest, const char *src) {
	size_t i, j;

	// Get end of destination string.
	for (i = 0; dest[i] != '\0'; i++);

	// Copy source onto end of destination.
	for (j = 0; src[j] != '\0'; j++)
		dest[i+j] = src[j];

	// Null terminate new string.
	dest[i+j] = '\0';
	return dest;
}

// Compares two strings.
int32_t strcmp(const char *str1, const char *str2) {
	// Compare the strings.
    while (*str1 && *str1 == *str2)
	{
		str1++;
		str2++;
	}

	// Return result.
	return *str1 - *str2;
}

// Compares two strings.
int32_t strncmp(const char *str1, const char *str2, size_t n) {
	// Compare the strings.
    while (*str1 && *str1 == *str2 && n)
	{
		n--;
		str1++;
		str2++;
	}

	// Return result.
	return *str1 - *str2;
}

// Copies one string to another.
char* strcpy(char *dest, const char *src) {
	// Copy the entire string.
	char *ret = dest;
	while ((*dest++ = *src++));
	return ret;
}

// Copies up to n character in one string to another.
char* strncpy(char *dest, const char *src, size_t n) {
	// Copy string until we reach the number of characters desired.
	char *ret = dest;
	while (n--)
		*dest++ = *src++;
	return ret;
}

// Gets the length of the specified string.
size_t strlen(const char *str) {
    // Count characters in string.
	size_t len = 0;
	while (str[len])
        len++;
	return len;
}


int32_t toupper(int32_t c) {
	if (c >= 97 && c <= 122)
		c -= 32;
	return c;
}

int32_t tolower(int32_t c) {
	if (c >= 65 && c <= 90)
		c += 32;
	return c;
}

int32_t isdigit(int32_t c) {
	if (c >= 48 && c <= 57)
		return 1;
	return 0;
}

int32_t isxdigit(int32_t c) {
	if ((c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102))
		return 1;
	return 0;
}

int32_t isspace(int32_t c) {
	if (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r')
		return 1;
	return 0;
}

int32_t isprint(int32_t c) {
	if (c >= 31 && c <= 126)
		return 1;
	return 0;
}