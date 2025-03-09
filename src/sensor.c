#include "sensor.h"
#include "clock_server.h"
#include "common.h"
#include "display_state_server.h"
#include "marklin.h"
#include "name_server.h"
#include "rpi.h"
#include "state_server.h"
#include "syscall_func.h"
#include "timer.h"
#include "track_data.h"
#include "train.h"
#include "uart_server.h"

#if defined(MEASURE_TRAIN_SPEED)
int clock_index(int bank, int sensor)
{
    int banks[8] = { 0, 2, 4, 3, 3, 4, 2, 1 };
    int sensors[8] = { 3, 13, 7, 7, 9, 12, 6, 15 };
    for (int i = 0; i < 8; ++i) {
        if (bank == banks[i] && sensor == sensors[i]) {
            return i;
        }
    }
    return -1;
}
#endif

int start_time, end_time;

void temp_task()
{
    delay(240);
    marklin_set_speed(55, 10);
    start_time = timer_get_ms();

    syscall_exit();
}

// distance=2408mm
// removing stop component, distance=2068mm,duration=9915ms

void sensor_task()
{
    char sensordata[10];
    memset(sensordata, 0, 10);

    track_node track[TRACK_MAX];
#ifdef TRACKA
    init_tracka(track);
#else
    init_trackb(track);
#endif

#if defined(MEASURE_TRAIN_SPEED)
    // A3 C13 E7 D7 D9 E12 C6 B15
    int clocks[8], duration[8], num_loops[8];
    memset(clocks, 0, sizeof(clocks));
    memset(duration, 0, sizeof(duration));
    memset(num_loops, 0, sizeof(num_loops));

    int distances[8] = { 581, 875, 384, 780, 369, 1008, 483, 437 };
    int global_duration = 0;
    int global_distance = 0;
    int global_num_loops = 0;
#endif

    // wait for display server to init switches
    display_lazy();

    for (;;) {
        int64_t res = putc(MARKLIN, 0x85);
        ASSERT(res >= 0, "putc failed");

        for (int i = 0; i < 10; ++i) {
            res = getc(MARKLIN);
            ASSERT(res >= 0, "getc failed");
            sensordata[i] = res;
        }

        for (int bank = 0; bank < NUM_BANKS; ++bank) {
            for (int i = 0; i < NUM_SENSORS_PER_BANK; ++i) {
                int byte_index = 2 * bank + i / 8;
                char byte = sensordata[byte_index];
                unsigned int bit = byte & (1 << (7 - (i % 8)));
                if (bit) {
                    state_set_recent_sensor(bank, i + 1);

                    char sensor_name[4];
                    sensor_name[0] = 'A' + bank;
                    if (i + 1 >= 10) {
                        sensor_name[1] = '0' + (i + 1) / 10;
                        sensor_name[2] = '0' + (i + 1) % 10;
                        sensor_name[3] = 0;
                    } else {
                        sensor_name[1] = '0' + (i + 1);
                        sensor_name[2] = 0;
                    }
                    train_sensor_reading(track, sensor_name);

                    if (strcmp(sensor_name, "E11") == 0) {
                        marklin_set_speed(55, 0);
                        create(50, temp_task);
                    }
                    if (strcmp(sensor_name, "C14") == 0) {
                        end_time = timer_get_ms();
                        printf(CONSOLE, "start: %d, end: %d\r\n", start_time, end_time);
                        marklin_set_speed(55, 0);
                    }

#if defined(MEASURE_TRAIN_SPEED)
                    int idx = clock_index(bank, i + 1);
                    if (idx < 0)
                        continue;
                    int clock_to_stop = (idx - 1 + 8) % 8;
                    if (clocks[clock_to_stop] > 0) {
                        num_loops[clock_to_stop]++;

                        if (num_loops[clock_to_stop] >= 2) {
                            uint64_t cur_duration = time() - clocks[clock_to_stop];
                            duration[clock_to_stop] += cur_duration;
                            global_duration += cur_duration;
                            global_distance += distances[clock_to_stop];
                            global_num_loops++;

                            int avg_speed_mm_s = distances[clock_to_stop] * (num_loops[clock_to_stop] - 1) * 100 / duration[clock_to_stop]; // mm/tick
                            int avg_speed_tot_mm_s = global_distance * 100 / global_duration;
                            printf(CONSOLE, "End sensor: %s, Num loops: %d, Avg velocity: %d, Total avg velocity: %d\r\n", sensor_name, num_loops[clock_to_stop], avg_speed_mm_s, avg_speed_tot_mm_s);
                        }

                        clocks[clock_to_stop] = 0;
                    }
                    if (clocks[idx] == 0) {
                        clocks[idx] = time();
                    }
#endif
                }
            }
        }
        display_lazy();
        res = delay(2);
        ASSERTF(res >= 0, "delay failed, %d", res);
    }
}
