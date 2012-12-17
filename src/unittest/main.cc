// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "utils.hpp"
#include "unittest/gtest.hpp"

int main(int argc, char **argv) {
    run_generic_global_startup_behavior();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
