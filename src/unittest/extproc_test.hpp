// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef UNITTEST_EXTPROC_TEST_HPP_
#define UNITTEST_EXTPROC_TEST_HPP_

#include "unittest/unittest_utils.hpp"

#define SPAWNER_TEST(group, name) void run_##group##_##name();  \
    TEST(group, name) {                                         \
        extproc_spawner_t extproc_spawner;                      \
        ::unittest::run_in_thread_pool(run_##group##_##name);   \
    }                                                           \
    void run_##group##_##name()

#endif  // UNITTEST_EXTPROC_TEST_HPP_
