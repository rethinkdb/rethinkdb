// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "utils.hpp"
#include "unittest/gtest.hpp"

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
    startup_shutdown_t startup_shutdown;
	open_console();
	defer_t defer(close_console);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
