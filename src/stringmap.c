#include "stringmap.h"
#include "common.h"
#include "rpi.h"

stringmap_t stringmap_new()
{
    stringmap_t map;
    map.num_keys = 0;
    return map;
}

int _stringmap_key_index(stringmap_t* map, char* key)
{
    for (int i = 0; i < map->num_keys; ++i) {
        if (strcmp(map->keys[i], key) == 0) {
            return i;
        }
    }
    return -1;
}

void stringmap_set(stringmap_t* map, char* key, uint64_t value)
{
    ASSERT(strlen(key) < MAX_KEY_SIZE, "key is too long.");
    int key_index = _stringmap_key_index(map, key);
    if (key_index == -1) {
        ASSERT(map->num_keys < NUM_TASKS, "stringmap full");
        key_index = map->num_keys;
        map->num_keys++;
    }
    strcpy(map->keys[key_index], key);
    map->values[key_index] = value;
}

uint64_t stringmap_get(stringmap_t* map, char* key)
{
    int key_index = _stringmap_key_index(map, key);
    ASSERT(key_index != -1, "key not found");
    return map->values[key_index];
}

void stringmap_remove(stringmap_t* map, char* key)
{
    int key_index = _stringmap_key_index(map, key);
    ASSERT(key_index != -1, "key not found");
    map->num_keys--;
    if (key_index < map->num_keys){
        strcpy(map->keys[key_index], map->keys[map->num_keys]);
        map->values[key_index] = map->values[map->num_keys];
    }
}