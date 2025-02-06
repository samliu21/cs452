#include "charqueue.h"
#include "rpi.h"
#include "util.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

charqueue charqueue_new(charqueuenode* available_nodes, int size)
{
    charqueue q;
    q.head = NULL;
    q.tail = NULL;
    q.available_nodes = available_nodes;
    q.available_nodes_pos = 0;
    q.size = size;
    return q;
}

int charqueue_empty(charqueue* q)
{
    return q->head == NULL;
}

void charqueue_add(charqueue* q, char c)
{
    charqueuenode* n = &q->available_nodes[q->available_nodes_pos++];
    n->data = c;
    n->next = NULL;
    if (q->head == NULL) {
        q->head = n;
        q->tail = n;
    } else {
        q->tail->next = n;
        q->tail = n;
    }
    q->available_nodes_pos %= q->size;
}

void charqueue_add_s(charqueue* q, char* s)
{
    while (*s != '\0') {
        charqueue_add(q, *s++);
    }
}

void charqueue_add_sf(charqueue* q, const char* s, ...)
{
    va_list va;
    char ch, buf[12];

    va_start(va, s);
    while ((ch = *(s++))) {
        if (ch != '%') {
            charqueue_add(q, ch);
        } else {
            ch = *(s++);
            switch (ch) {
            case 'u':
                ui2a(va_arg(va, unsigned int), 10, buf);
                charqueue_add_s(q, buf);
                break;
            case 'd':
                i2a(va_arg(va, int), buf);
                charqueue_add_s(q, buf);
                break;
            case 'c':
                charqueue_add(q, (char)va_arg(va, int));
                break;
            case 's':
                charqueue_add_s(q, va_arg(va, char*));
                break;
            case '\0':
                return;
            }
        }
    }
    va_end(va);
}

char charqueue_pop(charqueue* q)
{
    ASSERT(q->head != NULL, "popping from empty queue");
    char c = q->head->data;
    q->head = q->head->next;
    return c;
}

char charqueue_peek(charqueue* q)
{
    return q->head->data;
}
