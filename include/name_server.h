#ifndef _name_server_h_
#define _name_server_h_

#include "common.h"

#define WHO_IS 'W'
#define REGISTER_AS 'R'
#define NAME_SERVER_TID 5000

int64_t register_as(const char* name);
int64_t who_is(const char* name);

#endif
