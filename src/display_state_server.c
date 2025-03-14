#include "display_state_server.h"

#include "clock_server.h"
#include "marklin.h"
#include "name_server.h"
#include "state_server.h"
#include "switch.h"
#include "syscall_func.h"
#include "train.h"
#include "uart_server.h"

typedef enum {
    LAZY = 1,
    FORCE = 2,
} display_request_t;

void display(char type)
{
    int64_t display_state_task_tid = who_is(DISPLAY_STATE_TASK_NAME);
    ASSERT(display_state_task_tid >= 0, "who_is failed");

    int64_t ret = send(display_state_task_tid, &type, 1, NULL, 0);
    ASSERT(ret >= 0, "send failed");
}

void display_lazy()
{
    display(LAZY);
}

void display_force()
{
    display(FORCE);
}

void display_state_task()
{
    register_as(DISPLAY_STATE_TASK_NAME);

    track_node track[TRACK_MAX];
    #ifdef TRACKA
    init_tracka(track);
    #else
    init_trackb(track);
    #endif

    uint64_t old_usage = 0;
    uint64_t old_ticks = 0;
    char old_sensors[128];
    memset(&old_sensors, 0, 128);
    // initialize to be different from the initial sensor data
    old_sensors[0] = 255;
    char old_switches[128];
    memset(&old_switches, 0, 128);

    char old_train_times[256];
    memset(&old_train_times, 0, 256);
    old_train_times[0] = 255;

    char old_reservations[1024];
    memset(&old_reservations, 0, 1024);
    old_reservations[0] = 255;

    int old_train_cur_node = 0;
    int old_train_cur_offset = 0;

    char c;
    uint64_t notifier_tid;
    for (;;) {
        receive(&notifier_tid, &c, 1);
        reply_empty(notifier_tid);

        // perf
        uint64_t usage = cpu_usage();
        if (c == FORCE || usage != old_usage) {
            uint64_t kernel_percentage = usage % 100;
            uint64_t idle_percentage = usage / 100;
            uint64_t user_percentage = 100 - kernel_percentage - idle_percentage;
            char* format = "\033[s\033[1;1H\033[2Kkernel: %02u%%, idle: %02u%%, user: %02u%%\033[u";
            printf(CONSOLE, format, kernel_percentage, idle_percentage, user_percentage);
            old_usage = usage;
        }

        // clock
        uint64_t ticks = time();
        if (c == FORCE || ticks != old_ticks) {
            uint64_t minutes = ticks / 6000;
            uint64_t seconds = (ticks % 6000) / 100;
            uint64_t tenths = (ticks % 100) / 10;
            printf(CONSOLE, "\033[s\033[2;1H\033[2K%02d:%02d:%02d\033[u", minutes, seconds, tenths);
            old_ticks = ticks;
        }

        // sensors
        char sensors[128];
        state_get_recent_sensors(sensors);
        if (c == FORCE || strcmp(sensors, old_sensors)) {
            printf(CONSOLE, "\033[s\033[3;1H\033[2Ksensors: [ %s]\033[u", sensors);
            strcpy(old_sensors, sensors);
        }

        // switches
        char switches[128];
        state_get_switches(switches);
        if (c == FORCE || strcmp(switches, old_switches)) {
            printf(CONSOLE, "\033[s\033[4;1H\033[2Kswitches: [ %s]\033[u", switches);
            strcpy(old_switches, switches);
        }

        // train times
        char train_times[256];
        train_get_times(train_times);
        if (c == FORCE || strcmp(train_times, old_train_times)) {
            if (strlen(train_times) == 0) {
                puts(CONSOLE, "\033[s\033[5;1H\033[2Kdistance delta: n/a\033[u");
            } else {
                printf(CONSOLE, "\033[s\033[5;1H\033[2K%s\033[u", train_times);
            }
            strcpy(old_train_times, train_times);
        }

        // node + offset
        int train_cur_node = train_get_cur_node(55);
        int train_cur_offset = train_get_cur_offset(55);
        if (c == FORCE || train_cur_node != old_train_cur_node || train_cur_offset != old_train_cur_offset) {
            printf(CONSOLE, "\033[s\033[6;1H\033[2Ktrain 55 is at node: %s, and offset: %d\033[u", track[train_cur_node].name, train_cur_offset);
            old_train_cur_node = train_cur_node;
            old_train_cur_offset = train_cur_offset;
        }

        // reservations
        char reservations[1024];
        state_get_reservations(reservations);
        if (c == FORCE || strcmp(reservations, old_reservations)) {
            printf(CONSOLE, "\033[s\033[7;1H\033[2Kreservations: [ %s]\033[u", reservations);
            strcpy(old_reservations, reservations);
        }
    }
}

void display_state_notifier()
{
    // wait for display server to make sure all servers are ready.
    display_lazy();

    int64_t ret;
    int64_t ticks = time();
    for (;;) {
        ticks += 5;
        ret = delay_until(ticks);
        ASSERTF(ret >= 0, "delay failed %d", ret);
        display_lazy();
    }
}
