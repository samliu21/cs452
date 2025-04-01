#include "display_state_server.h"

#include "clock_server.h"
#include "marklin.h"
#include "name_server.h"
#include "state_server.h"
#include "switch.h"
#include "syscall_func.h"
#include "track_seg.h"
#include "train.h"
#include "uart_server.h"

static char track_template[18][256] = {
    "            12        11",
    "–––———————–\033[33m/\033[37m—————————\033[33m/\033[37m—————————————————————————————––\\",
    "          /         /      13                 10      \\",
    "–––——————\033[33m/\033[37m 4       \033[33m/\033[37m—————————\033[33m\\\033[37m—————–––——–––––\033[33m/\033[37m—————————\033[33m\\\033[37m",
    "        /         / 14         \\           /          9 \\",
    "–––————/         |               \\   |   /               |",
    "                 |                 \\ | /                 |",
    "                 |              156 \033[33m|\033[37m|\033[33m|\033[37m 155              |",
    "                 |              153 \033[33m|\033[37m|\033[33m|\033[37m 154              |",
    "                 |                 / | \\                 |",
    "–––————\\         |               /   |   \\               |",
    "        \\         \\ 15      16 /           \\ 17       8 /",
    "–––——————\033[33m\\\033[37m 1       \033[33m\\\033[37m—————————\033[33m/\033[37m——————––––—––––\033[33m\\\033[37m—————————\033[33m/\033[37m",
    "          \\         \\                                 /",
    "–––————————\033[33m\\\033[37m 2       \\———————\033[33m\\\033[37m———————————————\033[33m/\033[37m————–——/",
    "            \\               6 \\             / 7",
    "–––——————————\033[33m\\\033[37m—————————————————\033[33m\\\033[37m———————————\033[33m/\033[37m————————————",
    "              3                 18        5"
};

static char scoreboard_template[6][64] = {
    "           Scoreboard         ",
    "------------------------------",
    "| train |  55  |  77  |  58  |",
    "------------------------------",
    "| score |   0  |   0  |   0  |",
    "------------------------------",
};

typedef enum {
    LAZY = 1,
    FORCE = 2,
} display_request_t;

void display(char type)
{
    int64_t display_state_task_tid = who_is(DISPLAY_STATE_TASK_NAME);
    ASSERT(display_state_task_tid >= 0, "who_is failed");

    int64_t ret = send(display_state_task_tid, &type, 1, NULL, 0);
    ASSERT(ret >= 0, "display send failed");
}

void display_lazy()
{
    display(LAZY);
}

void display_force()
{
    display(FORCE);
}

void update_track_diagram(char* switches)
{
    int x[256];
    int y[256];
    char* c[256];

    x[1] = 12;
    y[1] = 10;
    c[1] = "\\";
    x[2] = 14;
    y[2] = 12;
    c[2] = "\\";
    x[3] = 16;
    y[3] = 14;
    c[3] = "\\";
    x[4] = 3;
    y[4] = 10;
    c[4] = "/";
    x[5] = 16;
    y[5] = 44;
    c[5] = "/";
    x[6] = 14;
    y[6] = 30;
    c[6] = "\\";
    x[7] = 14;
    y[7] = 46;
    c[7] = "/";
    x[8] = 12;
    y[8] = 56;
    c[8] = "/";
    x[9] = 3;
    y[9] = 56;
    c[9] = "\\";
    x[10] = 3;
    y[10] = 46;
    c[10] = "/";
    x[11] = 1;
    y[11] = 22;
    c[11] = "/";
    x[12] = 1;
    y[12] = 12;
    c[12] = "/";
    x[13] = 3;
    y[13] = 30;
    c[13] = "\\";
    x[14] = 3;
    y[14] = 20;
    c[14] = "/";
    x[15] = 12;
    y[15] = 20;
    c[15] = "\\";
    x[16] = 12;
    y[16] = 30;
    c[16] = "/";
    x[17] = 12;
    y[17] = 46;
    c[17] = "\\";
    x[18] = 16;
    y[18] = 32;
    c[18] = "\\";
    x[153] = 8;
    y[153] = 37;
    c[153] = "|";
    x[154] = 8;
    y[154] = 39;
    c[154] = "|";
    x[155] = 7;
    y[155] = 39;
    c[155] = "|";
    x[156] = 7;
    y[156] = 37;
    c[156] = "|";

    for (int i = 0; i < 256; ++i) {
        if (switches[i]) {
            printf(CONSOLE, "\033[s\033[%d;%dH\033[%dm%s\033[37m\033[u", 15 + x[i], y[i], switches[i] == S ? 36 : 31, c[i]);
        }
    }
}

void print_track_diagram(char* switches)
{
    for (int i = 0; i < 18; ++i) {
        printf(CONSOLE, "\033[s\033[%d;1H%s\033[u", 15 + i, track_template[i]);
    }

    update_track_diagram(switches);
}

void print_scoreboard(char* scores)
{
    for (int i = 0; i < 6; ++i) {
        printf(CONSOLE, "\033[s\033[%d;1H%s\033[u", 35 + i, scoreboard_template[i]);
    }
    // printf(CONSOLE, "\033[s\033[39;1H\033[2K| score |   %d  |   %d  |   %d  |\033[u\r\n", scores[0], scores[1], scores[2]);
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
    old_sensors[0] = 255;

    char old_switches[256];
    memset(&old_switches, 0, 256);
    old_switches[0] = 255;

    char old_reservations_55[1024];
    memset(&old_reservations_55, 0, 1024);
    old_reservations_55[0] = 255;

    char old_reservations_58[1024];
    memset(&old_reservations_58, 0, 1024);
    old_reservations_58[0] = 255;

    char old_reservations_77[1024];
    memset(&old_reservations_77, 0, 1024);
    old_reservations_77[0] = 255;

    int old_cur_node_55 = 0;
    int old_cur_offset_55 = 0;
    int old_dest_55 = 0;
    int old_cur_node_58 = 0;
    int old_cur_offset_58 = 0;
    int old_dest_58 = 0;
    int old_cur_node_77 = 0;
    int old_cur_offset_77 = 0;
    int old_dest_77 = 0;

    char old_forbidden_segments[TRACK_SEGMENTS_MAX];
    memset(&old_forbidden_segments, 0, TRACK_SEGMENTS_MAX);
    old_forbidden_segments[0] = 255;

    char dummy_switches[256];
    memset(dummy_switches, 0, 256);
    print_track_diagram(dummy_switches);

    char old_scores[256];
    memset(old_scores, 0, 256);
    old_scores[0] = 255;

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
        char switches[256];
        state_get_switches(switches);
        if (c == FORCE || strcmp(switches, old_switches)) {
            char buf[256];
            buf[0] = 0;
            for (int i = 0; i < 256; ++i) {
                if (switches[i]) {
                    char t[8];
                    sprintf(t, "%d:%c; ", i, switches[i]);
                    strcat(buf, t);
                }
            }
            printf(CONSOLE, "\033[s\033[4;1H\033[2Kswitches: [ %s]\033[u", buf);
            strcpy(old_switches, switches);

            update_track_diagram(switches);
        }

        int player_train = train_get_player();

        // train 55 model
        int cur_node_55 = train_get_cur_node(55);
        int cur_offset_55 = train_get_cur_offset(55);
        int dest_55 = train_get_dest(55);
        if (c == FORCE || cur_node_55 != old_cur_node_55 || cur_offset_55 != old_cur_offset_55 || dest_55 != old_dest_55) {
            printf(CONSOLE, "\033[s\033[5;1H\033[2Ktrain 55 is at node: %s, and offset: %3d                    destination: %s\033[u", track[cur_node_55].name, cur_offset_55, player_train == 55 ? "player-controlled" : track[dest_55].name);
            old_cur_node_55 = cur_node_55;
            old_cur_offset_55 = cur_offset_55;
            old_dest_55 = dest_55;
        }

        // train 58 model
        int cur_node_58 = train_get_cur_node(58);
        int cur_offset_58 = train_get_cur_offset(58);
        int dest_58 = train_get_dest(58);
        if (c == FORCE || cur_node_58 != old_cur_node_58 || cur_offset_58 != old_cur_offset_58 || dest_58 != old_dest_58) {
            printf(CONSOLE, "\033[s\033[6;1H\033[2Ktrain 58 is at node: %s, and offset: %3d                    destination: %s\033[u", track[cur_node_58].name, cur_offset_58, player_train == 58 ? "player-controlled" : track[dest_58].name);
            old_cur_node_58 = cur_node_58;
            old_cur_offset_58 = cur_offset_58;
            old_dest_58 = dest_58;
        }

        // train 77 model
        int cur_node_77 = train_get_cur_node(77);
        int cur_offset_77 = train_get_cur_offset(77);
        int dest_77 = train_get_dest(77);
        if (c == FORCE || cur_node_77 != old_cur_node_77 || cur_offset_77 != old_cur_offset_77 || dest_77 != old_dest_77) {
            printf(CONSOLE, "\033[s\033[7;1H\033[2Ktrain 77 is at node: %s, and offset: %3d                    destination: %s\033[u", track[cur_node_77].name, cur_offset_77, player_train == 77 ? "player-controlled" : track[dest_77].name);
            old_cur_node_77 = cur_node_77;
            old_cur_offset_77 = cur_offset_77;
            old_dest_77 = dest_77;
        }

        // train 55 reservations
        char reservations_55[1024];
        state_get_reservations(reservations_55, 55);
        if (c == FORCE || strcmp(reservations_55, old_reservations_55)) {
            printf(CONSOLE, "\033[s\033[8;1H\033[2Kreservations for 55: [ %s]\033[u", reservations_55);
            strcpy(old_reservations_55, reservations_55);
        }

        // train 58 reservations
        char reservations_58[1024];
        state_get_reservations(reservations_58, 58);
        if (c == FORCE || strcmp(reservations_58, old_reservations_58)) {
            printf(CONSOLE, "\033[s\033[9;1H\033[2Kreservations for 58: [ %s]\033[u", reservations_58);
            strcpy(old_reservations_58, reservations_58);
        }

        // train 77 reservations
        char reservations_77[1024];
        state_get_reservations(reservations_77, 77);
        if (c == FORCE || strcmp(reservations_77, old_reservations_77)) {
            printf(CONSOLE, "\033[s\033[10;1H\033[2Kreservations for 77: [ %s]\033[u", reservations_77);
            strcpy(old_reservations_77, reservations_77);
        }

        // forbidden segments
        char forbidden_segments[TRACK_SEGMENTS_MAX];
        memset(forbidden_segments, 0, TRACK_SEGMENTS_MAX);
        state_get_forbidden_segments(forbidden_segments);
        if (c == FORCE || memcmp(forbidden_segments, old_forbidden_segments, TRACK_SEGMENTS_MAX)) {
            char forbidden_segments_text[1024];
            memset(forbidden_segments_text, 0, 1024);
            int text_index = 0;
            for (int i = 0; i < TRACK_SEGMENTS_MAX; ++i) {
                if (forbidden_segments[i] && i != TRACK_SEGMENTS_MAX) {
                    text_index += sprintf(forbidden_segments_text + text_index, "%d ", i);
                }
            }
            printf(CONSOLE, "\033[s\033[11;1H\033[2Kforbidden segments: [ %s]\033[u", forbidden_segments_text);
            memcpy(old_forbidden_segments, forbidden_segments, TRACK_SEGMENTS_MAX);
        }

        // track diagram
        if (c == FORCE) {
            // clear lines in between
            for (int i = 12; i < 15; ++i) {
                printf(CONSOLE, "\033[s\033[%d;1H\033[2K\033[u", i);
            }

            print_track_diagram(switches);
        }

        char scores[256];
        get_train_scores(scores);
        if (c == FORCE || memcmp(scores, old_scores, 256)) {
            for (int i = 33; i < 35; ++i) {
                printf(CONSOLE, "\033[s\033[%d;1H\033[2K\033[u", i);
            }

            print_scoreboard(scores);
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
