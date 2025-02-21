#ifndef _charqueue_h_
#define _charqueue_h_

typedef struct charqueuenode {
    char data;
    struct charqueuenode* next;
} charqueuenode;

typedef struct charqueue {
    charqueuenode* head;
    charqueuenode* tail;
    charqueuenode* available_nodes;
    int available_nodes_pos;
    int size;
    int max_size;
} charqueue;

charqueue charqueue_new(charqueuenode* available_nodes, int size);
int charqueue_empty(charqueue* q);
void charqueue_add(charqueue* q, char c);
void charqueue_add_s(charqueue* q, char* s);
void charqueue_add_sf(charqueue* q, const char* s, ...);
char charqueue_pop(charqueue* q);
char charqueue_peek(charqueue* q);

#endif
