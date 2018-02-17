#include <main.h>
#include <string.h>

size_t strlen(const char* str)
{
    // Count characters in string.
	size_t len = 0;
	while (str[len])
        len++;
	return len;
}

bool strcmp(const char* a, const char* b)
{
    // Get length of string.
    size_t len = strlen(a);

    // Verify strings are valid and lengths match.
    if (!a || !b || len != strlen(b) )
        return false;

    // Verify each character matches.
    for (size_t i = 0; i < len; i++ )
    {
        if ( a[i] != b[i] )
            return false;
    }

    return true;
}
