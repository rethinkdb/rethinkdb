// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "utils.hpp"
#include "unittest/gtest.hpp"

// TODO ATN

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

// TODO ATN
BOOL unittest_ctrl_c(DWORD type) {
    // TODO: abort other threads with stack trace
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) {
        crash("^C");
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    startup_shutdown_t startup_shutdown;
    //open_console();
    setvbuf(stderr, nullptr, _IONBF, 0);
    SetConsoleCtrlHandler(unittest_ctrl_c, true);
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    //close_console(); // TODO ATN
    return ret;
}
