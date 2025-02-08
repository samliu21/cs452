#include "common.h"
#include "timer.h"

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

uint64_t min(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}
