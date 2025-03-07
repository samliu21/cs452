#include "common.h"
#include "timer.h"
#include <stdarg.h>

void memcpy(void* dest, const void* src, int n)
{
    char* csrc = (char*)src;
    char* cdest = (char*)dest;
    for (int i = 0; i < n; i++) {
        cdest[i] = csrc[i];
    }
}

void memset(void* s, int c, int n)
{
    unsigned char* p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
}

void strcpy(char* dest, const char* src)
{
    while ((*(dest++) = *src++))
        ;
}

int strlen(const char* s)
{
    int len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n)
{
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    } else {
        return (*(unsigned char*)s1 - *(unsigned char*)s2);
    }
}

int isspace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int split(char* s, char strings[][32])
{
    int cnt = 0;
    while (1) {
        while (isspace(*s) && *s != '\0') {
            s++;
        }
        if (*s == '\0') {
            break;
        }

        int pos = 0;
        while (!isspace(*s) && *s != '\0') {
            strings[cnt][pos++] = *s++;
        }
        strings[cnt][pos] = '\0';
        cnt++;
    }
    return cnt;
}

static uint64_t seed = 123456789; // You can set this to any initial value

uint64_t myrand()
{
    seed = (1103515245 * seed + 12345) & 0x7FFFFFFF; // Linear congruential generator
    return (seed >> 16) & 0x7FFF; // Scale down to 0-32767
}

uint64_t max(uint64_t a, uint64_t b)
{
	return a > b ? a : b;
}

uint64_t min(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}

enum flag_itoa {
    FILL_ZERO = 1,
    PUT_PLUS = 2,
    PUT_MINUS = 4,
    BASE_2 = 8,
    BASE_10 = 16,
};

static char* sitoa(char* buf, unsigned int num, int width, enum flag_itoa flags)
{
    unsigned int base;
    if (flags & BASE_2)
        base = 2;
    else if (flags & BASE_10)
        base = 10;
    else
        base = 16;

    char tmp[32];
    char* p = tmp;
    do {
        int rem = num % base;
        *p++ = (rem <= 9) ? (rem + '0') : (rem + 'a' - 0xA);
    } while ((num /= base));
    width -= p - tmp;
    char fill = (flags & FILL_ZERO) ? '0' : ' ';
    while (0 <= --width) {
        *(buf++) = fill;
    }
    if (flags & PUT_MINUS)
        *(buf++) = '-';
    else if (flags & PUT_PLUS)
        *(buf++) = '+';
    do
        *(buf++) = *(--p);
    while (tmp < p);
    return buf;
}

int my_vsprintf(char* buf, const char* fmt, va_list va)
{
    char c;
    const char* save = buf;

    while ((c = *fmt++)) {
        int width = 0;
        enum flag_itoa flags = 0;
        if (c != '%') {
            *(buf++) = c;
            continue;
        }
    redo_spec:
        c = *fmt++;
        switch (c) {
        case '%':
            *(buf++) = c;
            break;
        case 'c':;
            *(buf++) = va_arg(va, int);
            break;
        case 'd':;
            int num = va_arg(va, int);
            if (num < 0) {
                num = -num;
                flags |= PUT_MINUS;
            }
            buf = sitoa(buf, num, width, flags | BASE_10);
            break;
        case 'u':
            buf = sitoa(buf, va_arg(va, unsigned int), width, flags | BASE_10);
            break;
        case 'x':
            buf = sitoa(buf, va_arg(va, unsigned int), width, flags);
            break;
        case 'b':
            buf = sitoa(buf, va_arg(va, unsigned int), width, flags | BASE_2);
            break;
        case 's':;
            const char* p = va_arg(va, const char*);
            if (p) {
                while (*p)
                    *(buf++) = *(p++);
            }
            break;
        case '0':
            if (!width)
                flags |= FILL_ZERO;
            // fall through
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            width = width * 10 + c - '0';
            goto redo_spec;
        case '*':
            width = va_arg(va, unsigned int);
            goto redo_spec;
        case '+':
            flags |= PUT_PLUS;
            goto redo_spec;
        case '\0':
        default:
            *(buf++) = '?';
        }
        width = 0;
    }
    *buf = '\0';
    return buf - save;
}

int sprintf(char* buf, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int ret = my_vsprintf(buf, fmt, va);
    va_end(va);
    return ret;
}
