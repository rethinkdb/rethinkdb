#ifndef UNITTEST_TPTEST_HPP_
#define UNITTEST_TPTEST_HPP_

#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

#define TPTEST(group, name, ...) void run_##group##_##name();           \
    TEST(group, name) {                                                 \
        ::unittest::run_in_thread_pool(run_##group##_##name, ##__VA_ARGS__);      \
    }                                                                   \
    void run_##group##_##name()

#endif  // UNITTEST_TPTEST_HPP_
