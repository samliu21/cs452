#ifndef _stringmap_test_h_
#define _stringmap_test_h_

#include "stringmap.h"
#include "testutils.h"

int _test_stringmap()
{
    stringmap_t map = stringmap_new();
    TEST_ASSERT(map.num_keys == 0);

    stringmap_set(&map, "key1", 1);
    TEST_ASSERT(map.num_keys == 1);
    TEST_ASSERT(stringmap_get(&map, "key1") == 1);

    stringmap_set(&map, "key2", 2);
    TEST_ASSERT(map.num_keys == 2);
    TEST_ASSERT(stringmap_get(&map, "key2") == 2);

    stringmap_set(&map, "key1", 100);
    TEST_ASSERT(map.num_keys == 2);
    TEST_ASSERT(stringmap_get(&map, "key1") == 100);

    stringmap_remove(&map, "key1");
    TEST_ASSERT(map.num_keys == 1);
    TEST_ASSERT(stringmap_get(&map, "key2") == 2);

    stringmap_remove(&map, "key2");
    TEST_ASSERT(map.num_keys == 0);

    stringmap_set(&map, "key2", 200);
    TEST_ASSERT(map.num_keys == 1);
    TEST_ASSERT(stringmap_get(&map, "key2") == 200);

    return 1;
}

int run_stringmap_tests()
{
    TEST_RUN(_test_stringmap);
	return 1;
}

#endif
