#include "train.h"
#include <stdlib.h>

trainlist_t train_createlist(train_t* trains)
{
    trainlist_t tlist;
    tlist.trains = trains;
    tlist.size = 0;
    return tlist;
}

void train_add(trainlist_t* tlist, uint64_t id)
{
    tlist->trains[tlist->size].id = id;
    tlist->trains[tlist->size].speed = 0;
    tlist->size++;
}

void train_set_speed(trainlist_t* tlist, uint64_t id, uint64_t speed)
{
    train_t* t = train_find(tlist, id);
    t->speed = speed;
}

train_t* train_find(trainlist_t* tlist, uint64_t id)
{
    for (int i = 0; i < tlist->size; ++i) {
        if (tlist->trains[i].id == id) {
            return &tlist->trains[i];
        }
    }
    return NULL;
}
