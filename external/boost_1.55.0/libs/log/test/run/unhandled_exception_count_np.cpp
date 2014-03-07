/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   unhandled_exception_count_np.cpp
 * \author Andrey Semashev
 * \date   20.04.2013
 *
 * \brief  This file contains tests for the unhandled_exception_count internal function.
 *
 * The unhandled_exception_count function is not for public use but its tests show
 * whether the library will be able to emit log records in contexts when exceptions are being thrown.
 *
 * This file contains checks that are compiler specific and not quite portable.
 */

#define BOOST_TEST_MODULE unhandled_exception_count_np

#include <boost/test/unit_test.hpp>
#include <boost/log/detail/unhandled_exception_count.hpp>

struct my_exception1 {};
struct my_exception2 {};

class exception_watcher2
{
    unsigned int& m_count;

public:
    explicit exception_watcher2(unsigned int& count) : m_count(count) {}
    ~exception_watcher2() { m_count = boost::log::aux::unhandled_exception_count(); }
};

class exception_watcher1
{
    unsigned int& m_count1;
    unsigned int& m_count2;

public:
    exception_watcher1(unsigned int& count1, unsigned int& count2) : m_count1(count1), m_count2(count2) {}
    ~exception_watcher1()
    {
        m_count1 = boost::log::aux::unhandled_exception_count();
        try
        {
            exception_watcher2 watcher2(m_count2);
            throw my_exception2();
        }
        catch (...)
        {
        }
    }
};

// Tests for unhandled_exception_count when used in nested destructors while an exception propagates
BOOST_AUTO_TEST_CASE(in_nested_destructors)
{
    const unsigned int root_count = boost::log::aux::unhandled_exception_count();

    unsigned int level1_count = root_count, level2_count = root_count;
    try
    {
        exception_watcher1 watcher1(level1_count, level2_count);
        throw my_exception1();
    }
    catch (...)
    {
    }

    BOOST_CHECK_NE(root_count, level1_count);
    BOOST_CHECK_NE(root_count, level2_count);
    BOOST_CHECK_NE(level1_count, level2_count);
}
