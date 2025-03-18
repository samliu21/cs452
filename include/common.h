#ifndef _common_h_
#define _common_h_

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef long long int64_t;

typedef void (*func_t)();

typedef struct pi_t {
    int weight, id;
    struct pi_t* next;
} pi_t;

#define NUM_TASKS 128
#define STACK_SIZE (2 * 1024 * 1024)
#define MAX_RAND 32767

void memcpy(void* dest, const void* src, int n);
void memset(void* s, int c, int n);
void strcpy(char* dest, const char* src);
int strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
void strcat(char* dest, const char* src);
int split(char* s, char strings[][32]);
uint64_t max(uint64_t a, uint64_t b);
uint64_t min(uint64_t a, uint64_t b);
uint64_t myrand();
int sprintf(char* buf, const char* fmt, ...);

#endif /* common.h */