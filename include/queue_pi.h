#ifndef _queue_h_
#define _queue_h_

#include "common.h"

typedef struct queue_pi_t {
    pi_t* head;
    pi_t* tail;
    int size;
} queue_pi_t;

queue_pi_t queue_pi_new();
void queue_pi_add(queue_pi_t* q, pi_t* pi);
pi_t* queue_pi_pop(queue_pi_t* q);
void queue_pi_delete(queue_pi_t* q, pi_t* pi);
pi_t* queue_pi_peek(queue_pi_t* q);
int queue_pi_empty(queue_pi_t* q);

#endif
