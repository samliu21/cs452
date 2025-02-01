#ifndef _uintmap_test_h_
#define _uintmap_test_h_

#include "testutils.h"
#include "uintmap.h"

int _test_uintmap()
{
    uintmap_t map = uintmap_new();
    TEST_ASSERT(map.num_keys == 0);

    uintmap_set(&map, 101, 1);
    TEST_ASSERT(map.num_keys == 1);
    TEST_ASSERT(uintmap_get(&map, 101) == 1);

    uintmap_set(&map, 102, 2);
    TEST_ASSERT(map.num_keys == 2);
    TEST_ASSERT(uintmap_get(&map, 102) == 2);

    uintmap_set(&map, 101, 100);
    TEST_ASSERT(map.num_keys == 2);
    TEST_ASSERT(uintmap_get(&map, 101) == 100);

    uintmap_remove(&map, 101);
    TEST_ASSERT(map.num_keys == 1);
    TEST_ASSERT(uintmap_get(&map, 102) == 2);

    uintmap_remove(&map, 102);
    TEST_ASSERT(map.num_keys == 0);

    uintmap_set(&map, 102, 200);
    TEST_ASSERT(map.num_keys == 1);
    TEST_ASSERT(uintmap_get(&map, 102) == 200);

    return 1;
}

int run_uintmap_tests()
{
    TEST_RUN(_test_uintmap);
    return 1;
}

#endif
