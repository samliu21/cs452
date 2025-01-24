#include "name_server.h"
#include "stringmap.h"
#include "syscall_asm.h"
#include "syscall_func.h"
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
    int ret = send(NAME_SERVER_TID, buf, buffersz, reply_buf, 4);

    if (ret < 0) {
        return -1;
    }
    uint64_t tid = a2ui(reply_buf, 10);
    return tid;
}

void k2_name_server()
{
    stringmap_t names;
    uint64_t caller_tid, mapped_tid;
    const int bufsize = 128;
    char namebuf[bufsize + 1];
    char argv[3][MAX_KEY_SIZE];

    for (;;) {
        int64_t sz = receive(&caller_tid, namebuf, bufsize);
        namebuf[sz] = 0;
        int argc = split(namebuf, argv);
        for (int i = 0; i < argc; ++i) {
            uart_printf(CONSOLE, "argv[%d]: %s\r\n", i, argv[i]);
        }

        switch (argv[0][0]) {
        case WHO_IS:
            ASSERT(argc == 2, "'who_is' takes 1 argument");
            mapped_tid = stringmap_get(&names, argv[1]);
            char buf[4];
            ui2a(mapped_tid, 10, buf);
            reply(caller_tid, buf, 4);
            break;
        case REGISTER_AS:
            ASSERT(argc == 2, "'register_as' takes 1 argument");
            stringmap_set(&names, argv[1], caller_tid);
            reply_null(caller_tid);
            break;
        default:
            ASSERT(0, "invalid command");
        }
    }
}
