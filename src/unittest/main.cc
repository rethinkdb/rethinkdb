// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "utils.hpp"
#include "unittest/gtest.hpp"

int main(int argc, char **argv) {
    startup_shutdown_t startup_shutdown;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
