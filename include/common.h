#ifndef _common_h_
#define _common_h_

typedef unsigned long long uint64_t;
typedef long long int64_t;

typedef void (*func_t)();

#define NUM_TASKS 16
#define STACK_SIZE 16 * 1024
#define MAX_RAND 32767

void memcpy(void* dest, const void* src, int n);
void strcpy(char* dest, const char* src);
int strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
int split(char* s, char strings[][32]);

uint64_t myrand();

#endif /* common.h */