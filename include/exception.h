#ifndef _exception_h_
#define _exception_h_

#include "common.h"

extern void init_vbar(uint64_t exception_handler, uint64_t dummy_handler);

void dummy_handler();

void exception_handler();

#endif /* exception.h */