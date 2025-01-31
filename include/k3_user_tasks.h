#ifndef _k3_user_tasks_
#define _k3_user_tasks_

#include "timer.h"

void k3_initial_user_task()
{
    timer_set_delay(2000);

    for (;;) { }
}

#endif
