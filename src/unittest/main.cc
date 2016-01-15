// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "utils.hpp"
#include "unittest/gtest.hpp"
#include "extproc/extproc_spawner.hpp"

#ifdef _WIN32
int unittest_main(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
    startup_shutdown_t startup_shutdown;

#ifdef _WIN32
    extproc_maybe_run_worker(argc, argv);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
