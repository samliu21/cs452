#ifndef _exception_h_
#define _exception_h_

#include "common.h"

extern void init_vbar();

// exception vector handlers
void dummy_handler();
void synchronous_kernel_error(uint64_t esr);

#endif /* exception.h */