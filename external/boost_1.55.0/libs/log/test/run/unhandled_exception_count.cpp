/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   unhandled_exception_count.cpp
 * \author Andrey Semashev
 * \date   20.04.2013
 *
 * \brief  This file contains tests for the unhandled_exception_count internal function.
 *
 * The unhandled_exception_count function is not for public use but its tests show
 * whether the library will be able to emit log records in contexts when exceptions are being thrown.
 *
 * This file only contains the very basic checks of functionality that can be portably achieved
 * through std::unhandled_exception.
 */

#define BOOST_TEST_MODULE unhandled_exception_count

#include <boost/test/unit_test.hpp>
#include <boost/log/detail/unhandled_exception_count.hpp>

struct my_exception {};

class exception_watcher
{
    unsigned int& m_count;

public:
    explicit exception_watcher(unsigned int& count) : m_count(count) {}
    ~exception_watcher() { m_count = boost::log::aux::unhandled_exception_count(); }
};

// Tests for unhandled_exception_count when used in a destructor while an exception propagates
BOOST_AUTO_TEST_CASE(in_destructor)
{
    const unsigned int root_count = boost::log::aux::unhandled_exception_count();

    unsigned int level1_count = root_count;
    try
    {
        exception_watcher watcher(level1_count);
        throw my_exception();
    }
    catch (...)
    {
    }

    BOOST_CHECK_NE(root_count, level1_count);
}
