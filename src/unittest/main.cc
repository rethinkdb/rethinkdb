// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "utils.hpp"
#include "unittest/gtest.hpp"
#include "extproc/extproc_spawner.hpp"

int main(int argc, char **argv) {

#ifdef _WIN32
    extproc_maybe_run_worker(argc, argv);
#endif

    startup_shutdown_t startup_shutdown;

    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();

    return ret;
}
