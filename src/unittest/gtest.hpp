#ifndef __UNITTEST_GTEST_HPP__
#define __UNITTEST_GTEST_HPP__

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

// gtest #includes <assert.h>, so we have to undefine the assert macro.

// TODO: Rename assert in errors.hpp
#undef assert


#endif  // __UNITTEST_GTEST_HPP__

