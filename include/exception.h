#ifndef _exception_h_
#define _exception_h_

#include "common.h"

extern void init_vbar();

void dummy_handler();

void exception_handler();

void synchronous_error(uint64_t esr);

#endif /* exception.h */