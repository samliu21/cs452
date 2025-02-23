#ifndef _priority_queue_pi_h_
#define _priority_queue_pi_h_

typedef struct pi_t {
    int weight, id;
    struct pi_t* next;
} pi_t;

typedef struct priority_queue_pi_t {
    pi_t* head;
    pi_t* tail;
    int size;
} priority_queue_pi_t;

priority_queue_pi_t pq_pi_new();
void pq_pi_add(priority_queue_pi_t* pq, pi_t* pi);
pi_t* pq_pi_pop(priority_queue_pi_t* pq);
pi_t* pq_pi_peek(priority_queue_pi_t* pq);
int pq_pi_empty(priority_queue_pi_t* pq);
void pq_pi_debug(priority_queue_pi_t* pq);

#endif
