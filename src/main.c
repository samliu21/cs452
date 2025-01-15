#include "rpi.h"
#include "task.h"
#include "stack.h"

void foo() {
	uart_puts(CONSOLE, "Hello world!\r\n");
}

int kmain() {
	gpio_init();
	uart_config_and_enable(CONSOLE);
	uart_puts(CONSOLE, "\r\nHello world, this is version: " __DATE__ " / " __TIME__ "\r\n\r\nPress 'q' to reboot\r\n");

	char stack[NUM_TASKS * STACK_SIZE];
	task_t task = task_new(priority_0, (uint64_t)stack, &foo);
	task_run(task);
}
