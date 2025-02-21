#ifndef _display_state_server_h_
#define _display_state_server_h_

typedef enum {
    LAZY = 1,
    FORCE = 2,
} display_request_t;

void display_lazy();
void display_force();
void display_state_task();
void display_state_notifier();

#endif