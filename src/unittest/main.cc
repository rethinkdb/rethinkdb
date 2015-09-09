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

int main(int argc, char **argv) {
    printf("Starting RethinkDB unittest main.\n");
    startup_shutdown_t startup_shutdown;
    //open_console();
    //defer_t defer(close_console);
    ::testing::InitGoogleTest(&argc, argv);
    printf("Running tests\n");
    return RUN_ALL_TESTS();
}
