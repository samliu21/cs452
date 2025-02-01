#include "uintmap.h"
#include "common.h"
#include "rpi.h"

uintmap_t uintmap_new()
{
    uintmap_t map;
    map.num_keys = 0;
    return map;
}

int _uintmap_key_index(uintmap_t* map, uint64_t key)
{
    for (int i = 0; i < map->num_keys; ++i) {
        if (map->keys[i] == key) {
            return i;
        }
    }
    return -1;
}

void uintmap_set(uintmap_t* map, uint64_t key, uint64_t value)
{
    int key_index = _uintmap_key_index(map, key);
    if (key_index == -1) {
        ASSERT(map->num_keys < NUM_TASKS, "uintmap full");
        key_index = map->num_keys;
        map->num_keys++;
    }
    map->keys[key_index] = key;
    map->values[key_index] = value;
}

uint64_t uintmap_get(uintmap_t* map, uint64_t key)
{
    int key_index = _uintmap_key_index(map, key);
    ASSERT(key_index != -1, "key not found");
    return map->values[key_index];
}

void uintmap_remove(uintmap_t* map, uint64_t key)
{
    int key_index = _uintmap_key_index(map, key);
    ASSERT(key_index != -1, "key not found");
    map->num_keys--;
    if (key_index < map->num_keys){
        map->keys[key_index] = map->keys[map->num_keys];
        map->values[key_index] = map->values[map->num_keys];
    }
}