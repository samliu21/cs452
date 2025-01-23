#include "name_server.h"
#include "syscall.h"
#include <stdlib.h>

int64_t register_as(const char* name)
{
    int sz = strlen(name);
    int buffersz = sz + 3;
    char buf[buffersz];
    buf[0] = REGISTER_AS;
    buf[1] = ' ';
    memcpy(buf + 2, name, sz);
    buf[sz + 2] = 0;

    int ret = send(NAME_SERVER_TID, buf, buffersz, NULL, 0);

    if (ret < 0) {
        return -1;
    }
    return 0;
}

int64_t who_is(const char* name)
{
    int sz = strlen(name);
    int buffersz = sz + 3;
    char buf[buffersz];
    buf[0] = WHO_IS;
    buf[1] = ' ';
    memcpy(buf + 2, name, sz);
    buf[sz + 2] = 0;

    char reply_buf[4]; // tid
    int ret = send(0, buf, buffersz, reply_buf, 4); // TODO: how to get tid?

    if (ret < 0) {
        return -1;
    }
    return 0;
}
