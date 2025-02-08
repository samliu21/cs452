#ifndef _sensor_h_
#define _sensor_h_

#include "common.h"
#include "name_server.h"
#include "rpi.h"
#include "state_server.h"
#include "uart_server.h"

#define NUM_BANKS 5
#define NUM_SENSORS_PER_BANK 16

void sensor_task()
{
    int64_t state_task_tid = who_is(STATE_TASK_NAME);
    ASSERT(state_task_tid >= 0, "who_is failed");
    int64_t marklin_task_tid = who_is(MARKLIN_TASK_NAME);
    ASSERT(marklin_task_tid >= 0, "who_is failed");
    int64_t clock_task_tid = who_is(CLOCK_TASK_NAME);
    ASSERT(clock_task_tid >= 0, "who_is failed");

    char sensordata[10];
    memset(sensordata, 0, 10);
	uint64_t last_time;
	
    for (;;) {
        uart_puts(CONSOLE, "sending char\r\n");
        int64_t res = putc(marklin_task_tid, MARKLIN, 0x85);
        ASSERT(res >= 0, "putc failed");

        uart_puts(CONSOLE, "reading char\r\n");
        for (int i = 0; i < 10; ++i) {
            res = getc(marklin_task_tid, MARKLIN);
            ASSERT(res >= 0, "getc failed");
            sensordata[i] = res;
        }
        uart_puts(CONSOLE, "char has been read\r\n");

        for (int bank = 0; bank < NUM_BANKS; ++bank) {
            for (int i = 0; i < NUM_SENSORS_PER_BANK; ++i) {
                int byte_index = 2 * bank + i / 8;
                char byte = sensordata[byte_index];
                unsigned int bit = byte & (1 << (7 - (i % 8)));
                if (bit) {
                    state_set_recent_sensor(state_task_tid, bank, i);
                }
            }
        }
        uart_puts(CONSOLE, "sensor task done\r\n");

		last_time = timer_get_ms();
        while (timer_get_ms() - last_time < 100) {}
    }
}

#endif
