#include <main.h>
#include <kprint.h>
#include <driver/serial.h>
#include <driver/vga.h>
#include <kernel/lock.h>

lock_t kprintf_mutex = 0;

// Print a single character.
void kputchar(char c)
{
    // Print the character to both the screen and the serial.
    serial_write(c);
    vga_putchar(c);
}

// Print a hexadecimal byte.
void kputchar_hex(uint8_t num, bool capital, bool pad)
{
    // Get high and low nibbles.
    const uint8_t high = (num & 0xF0) >> 4;
    const uint8_t low = num & 0x0F;

    // Our hexadecimal ASCII dictionary.
    const char* hex_chars = capital ? "0123456789ABCDEF" : "0123456789abcdef";

    // Print the hex number.
    if (high != 0 || pad)
        kputchar(hex_chars[high]);
    kputchar(hex_chars[low]);
}

// Print a string.
void kputstring(const char *str, size_t max)
{
    // Print out string.
    if (max == 0) {
        while (*str) {
            kputchar(*str);
            str++;
        }
    }
    else {
        size_t len = strlen(str);
        
        while (*str && max) {
            kputchar(*str);
            str++;
            max--;
        }
    }
}

// Print an integer.
void kprint_int(int64_t num)
{
    // If zero, just print zero.
    if (num == 0)
    {
        kputchar('0');
        return;
    }

    // Determine if negative and get absolute value.
    bool negative = num < 0;
    num = negative ? -num : num;

    // Create buffer.
    char buffer[1 + 20 + 1]; // Sign, maximum size of 64-bit int, null terminator.
    buffer[sizeof(buffer) - 1] = '\0';

    // Walk through the number backwards.
    int32_t i = sizeof(buffer) - 2;
    while (num > 0)
    {
        buffer[i--] = '0' + (num % 10);
        num /= 10;
    }

    // Add sign if negative.
    if (negative)
        buffer[i--] = '-';

    // Print out numeral.
    kputstring(&buffer[i + 1], 0);
}

// Print an unsigned integer.
void kprint_uint(uint64_t num)
{
    // If zero, just print zero.
    if (num == 0)
    {
        kputchar('0');
        return;
    }

    // Create buffer.
    char buffer[20 + 1]; // Maximum size of unsigned 64-bit int, null terminator.
    buffer[sizeof(buffer) - 1] = '\0';

    // Walk through the number backwards.
    int32_t i = sizeof(buffer) - 2;
    while (num > 0)
    {
        buffer[i--] = '0' + (num % 10);
        num /= 10;
    }

    // Print out numeral.
    kputstring(&buffer[i + 1], 0);
}

// Print unsigned int as hexadecimal.
void kprint_hex(uint64_t num, bool capital, bool pad)
{
    // If zero, just print zero.
    if (num == 0)
    {
        kputchar('0');
        return;
    }

    bool first = true;

    for (int32_t i = sizeof(num) - 1; i >= 0; i--)
    {
        const uint8_t byte =(num >> (8 * i)) & 0xFF;

        if (first && byte == 0)
            continue;

        // Print hex byte.
        kputchar_hex(byte, capital, !first ? true : pad);
        first = false;
    }
}

void kprintf(const char* format, ...) {
    spinlock_lock(&kprintf_mutex);

    // Get args.
    va_list args;
    va_start(args, format);

    // Call va_list kprintf.
    kprintf_va(format, args);

    spinlock_release(&kprintf_mutex);
}

void kprintf_nlock(const char* format, ...) {
    // Get args.
    va_list args;
    va_start(args, format);

    // Call va_list kprintf.
    kprintf_va(format, args);
}

// https://en.wikipedia.org/wiki/Printf_format_string
// Printf implementation.
void kprintf_va(const char* format, va_list args) {
    // Disable cursor for increased performance.
    vga_disable_cursor();

    // Iterate through format string.
    char c;
    while (*format)
    {
        // Get current character.
        c = *format++;

        // Do we have the start of a variable?
        if (c == '%')
        {
            // Get type of formatting.
            char f = *format++;

            // Ignore justify for now.
            if (f == '-')
                f = *format++;

            // Check width.
            size_t width = 0;
            if (f >= '0' && f <= '9') {
                //width += f - '0';
                f = *format++;
            }

            // Ignore justify for now.
            if (f == '.')
                f = *format++;

            // Check width.
            if (f >= '0' && f <= '9') {
                width += f - '0';
                f = *format++;
            }

            // Do we have a long long?
            if (f == 'l' && *format++ == 'l')
            {
                // Handle 32-bit integers.
                switch (*format++)
                {
                    // If we have a null, skip over.
                    case '\0':
                        break;
                    
                    // If we have a %, print a literal %.
                    case '%':
                        kputchar('%');
                        break;

                    // Print integer.
                    case 'd':
                    case 'i':
                        kprint_int((int64_t)va_arg(args, int64_t));
                        break;

                    // Print unsigned integer.
                    case 'u':
                        kprint_uint((uint64_t)va_arg(args, uint64_t));
                        break;

                    // Print floating point.
                    case 'f':
                    case 'F':
                        // TODO.
                        break;
                    
                    // Print hexadecimal.
                    case 'x':
                        kprint_hex((uint64_t)va_arg(args, uint64_t), false, false);
                        break;

                    // Print hexadecimal (uppercase).
                    case 'X':
                        kprint_hex((uint64_t)va_arg(args, uint64_t), true, false);
                        break;

                    // Print string.
                    case 's':
                        kputstring((const char*)va_arg(args, const char*), width);
                        break;

                    // Print character.
                    case 'c':
                        kputchar((char)va_arg(args, int32_t));
                        break;
                }
            }
            else
            {
                // Handle 32-bit integers.
                switch (f)
                {
                    // If we have a null, skip over.
                    case '\0':
                        break;
                    
                    // If we have a %, print a literal %.
                    case '%':
                        kputchar('%');
                        break;

                    // Print integer.
                    case 'd':
                    case 'i':
                        kprint_int((int32_t)va_arg(args, int32_t));
                        break;

                    // Print unsigned integer.
                    case 'u':
                        kprint_uint((uint32_t)va_arg(args, uint32_t));
                        break;

                    // Print floating point.
                    case 'f':
                    case 'F':
                        // TODO.
                        break;
                    
                    // Print hexadecimal.
                    case 'x':
                        kprint_hex((uint32_t)va_arg(args, uint32_t), false, false);
                        break;

                    // Print hexadecimal (uppercase).
                    case 'X':
                        kprint_hex((uint32_t)va_arg(args, uint32_t), true, false);
                        break;

                    // Print pointer as hex.
                    case 'p':
                    case 'P':
                        kprint_hex((uintptr_t)va_arg(args, uintptr_t), true, false);
                        break;

                    // Print string.
                    case 's':
                        kputstring((const char*)va_arg(args, const char*), width);
                        break;

                    // Print character.
                    case 'c':
                        kputchar((char)va_arg(args, int32_t));
                        break;
                }
            }       
        }
        else
        {
            // Print any other characters.
            kputchar(c);
        }
    }

    // Re-enable the console driver
    vga_enable_cursor();
}
