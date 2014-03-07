// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2008 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/detail/config.hpp>

#include <boost/thread/xtime.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

void test_xtime_cmp()
{
    boost::xtime xt1, xt2, cur;
    BOOST_CHECK_EQUAL(
        boost::xtime_get(&cur, boost::TIME_UTC_),
        static_cast<int>(boost::TIME_UTC_));

    xt1 = xt2 = cur;
    xt1.nsec -= 1;
    xt2.nsec += 1;

    BOOST_CHECK(boost::xtime_cmp(xt1, cur) < 0);
    BOOST_CHECK(boost::xtime_cmp(xt2, cur) > 0);
    BOOST_CHECK(boost::xtime_cmp(cur, cur) == 0);

    xt1 = xt2 = cur;
    xt1.sec -= 1;
    xt2.sec += 1;

    BOOST_CHECK(boost::xtime_cmp(xt1, cur) < 0);
    BOOST_CHECK(boost::xtime_cmp(xt2, cur) > 0);
    BOOST_CHECK(boost::xtime_cmp(cur, cur) == 0);
}

void test_xtime_get()
{
    boost::xtime orig, cur, old;
    BOOST_CHECK_EQUAL(
        boost::xtime_get(&orig,
            boost::TIME_UTC_), static_cast<int>(boost::TIME_UTC_));
    old = orig;

    for (int x=0; x < 100; ++x)
    {
        BOOST_CHECK_EQUAL(
            boost::xtime_get(&cur, boost::TIME_UTC_),
            static_cast<int>(boost::TIME_UTC_));
        BOOST_CHECK(boost::xtime_cmp(cur, orig) >= 0);
        BOOST_CHECK(boost::xtime_cmp(cur, old) >= 0);
        old = cur;
    }
}

void test_xtime_mutex_backwards_compatibility()
{
    boost::timed_mutex m;
    BOOST_CHECK(m.timed_lock(boost::get_xtime(boost::get_system_time()+boost::posix_time::milliseconds(10))));
    m.unlock();
    boost::timed_mutex::scoped_timed_lock lk(m,boost::get_xtime(boost::get_system_time()+boost::posix_time::milliseconds(10)));
    BOOST_CHECK(lk.owns_lock());
    if(lk.owns_lock())
    {
        lk.unlock();
    }
    BOOST_CHECK(lk.timed_lock(boost::get_xtime(boost::get_system_time()+boost::posix_time::milliseconds(10))));
    if(lk.owns_lock())
    {
        lk.unlock();
    }
}

bool predicate()
{
    return false;
}


void test_xtime_condvar_backwards_compatibility()
{
    boost::condition_variable cond;
    boost::condition_variable_any cond_any;
    boost::mutex m;

    boost::unique_lock<boost::mutex> lk(m);
    cond.timed_wait(lk,boost::get_xtime(boost::get_system_time()+boost::posix_time::milliseconds(10)));
    cond.timed_wait(lk,boost::get_xtime(boost::get_system_time()+boost::posix_time::milliseconds(10)),predicate);
    cond_any.timed_wait(lk,boost::get_xtime(boost::get_system_time()+boost::posix_time::milliseconds(10)));
    cond_any.timed_wait(lk,boost::get_xtime(boost::get_system_time()+boost::posix_time::milliseconds(10)),predicate);
}



boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: xtime test suite");

    test->add(BOOST_TEST_CASE(&test_xtime_cmp));
    test->add(BOOST_TEST_CASE(&test_xtime_get));
    test->add(BOOST_TEST_CASE(&test_xtime_mutex_backwards_compatibility));
    test->add(BOOST_TEST_CASE(&test_xtime_condvar_backwards_compatibility));

    return test;
}
