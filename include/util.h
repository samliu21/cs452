#ifndef _util_h_
#define _util_h_

#include "common.h"

int a2d(char ch);
int a2i(char* s, unsigned int base);
unsigned int a2ui(char* s, unsigned int base);
void ui2a(unsigned int num, unsigned int base, char* bf);
void i2a(int num, char* bf);
void ui2cbuf(uint32_t num, char* bf);
uint32_t cbuf2ui(char* bf);

#endif /* util.h */
