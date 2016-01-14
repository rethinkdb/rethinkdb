// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "utils.hpp"
#include "unittest/gtest.hpp"
#include "extproc/extproc_spawner.hpp"

// TODO ATN

#ifdef _WIN32

void open_console() {
    AllocConsole();
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);
}

void close_console() {
    printf("Press return to exit...");
    fflush(stdout);
    getchar();
}

#endif

#ifdef _WIN32 // TODO ATN
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
