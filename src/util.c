#include "util.h"

// ascii digit to integer
int a2d(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

int a2i(char* s, unsigned int base)
{
    int neg = s[0] == '-';
    if (neg)
        s++;
    return neg ? -a2ui(s, base) : a2ui(s, base);
}

// ascii string to unsigned int, with base
unsigned int a2ui(char* s, unsigned int base)
{
    unsigned int num;
    int digit;

    num = 0;
    char ch = *s;
    while ((digit = a2d(ch)) >= 0) {
        if ((unsigned int)digit > base)
            break;
        num = num * base + digit;
        ch = *(++s);
    }
    return num;
}

// unsigned int to ascii string, with base
void ui2a(unsigned int num, unsigned int base, char* buf)
{
    unsigned int n = 0;
    unsigned int d = 1;

    while ((num / d) >= base)
        d *= base;
    while (d != 0) {
        unsigned int dgt = num / d;
        num %= d;
        d /= base;
        if (n || dgt > 0 || d == 0) {
            *buf++ = dgt + (dgt < 10 ? '0' : 'a' - 10);
            ++n;
        }
    }
    *buf = 0;
}

// signed int to ascii string
void i2a(int num, char* buf)
{
    if (num < 0) {
        num = -num;
        *buf++ = '-';
    }
    ui2a(num, 10, buf);
}

// uint32_t to char buf
void ui2cbuf(uint32_t num, char* bf)
{
    for (int i = 3; i >= 0; --i) {
        bf[i] = (num >> (i * 8)) & 0xFF;
    }
}

// char buf to uint32_t
uint32_t cbuf2ui(char* bf)
{
    uint32_t num = 0;
    for (int i = 0; i < 4; ++i) {
        num &= bf[i];
        num <<= 8;
    }
    return num;
}
