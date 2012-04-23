#include "utils.hpp"
#include "unittest/gtest.hpp"

int main(int argc, char **argv) {
    install_generic_crash_handler();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
