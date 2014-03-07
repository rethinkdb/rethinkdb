// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2
#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/thread/detail/config.hpp>

#include <boost/thread/condition.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/xtime.hpp>

#include <boost/test/unit_test.hpp>

#include "./util.inl"

struct condition_test_data
{
    condition_test_data() : notified(0), awoken(0) { }

    boost::mutex mutex;
    boost::condition_variable condition;
    int notified;
    int awoken;
};

void condition_test_thread(condition_test_data* data)
{
    boost::unique_lock<boost::mutex> lock(data->mutex);
    BOOST_CHECK(lock ? true : false);
    while (!(data->notified > 0))
        data->condition.wait(lock);
    BOOST_CHECK(lock ? true : false);
    data->awoken++;
}

struct cond_predicate
{
    cond_predicate(int& var, int val) : _var(var), _val(val) { }

    bool operator()() { return _var == _val; }

    int& _var;
    int _val;
private:
    void operator=(cond_predicate&);

};

void condition_test_waits(condition_test_data* data)
{
    boost::unique_lock<boost::mutex> lock(data->mutex);
    BOOST_CHECK(lock ? true : false);

    // Test wait.
    while (data->notified != 1)
        data->condition.wait(lock);
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 1);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait.
    data->condition.wait(lock, cond_predicate(data->notified, 2));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 2);
    data->awoken++;
    data->condition.notify_one();

    // Test timed_wait.
    boost::xtime xt = delay(10);
    while (data->notified != 3)
        data->condition.timed_wait(lock, xt);
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 3);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait.
    xt = delay(10);
    cond_predicate pred(data->notified, 4);
    BOOST_CHECK(data->condition.timed_wait(lock, xt, pred));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred());
    BOOST_CHECK_EQUAL(data->notified, 4);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait with relative timeout
    cond_predicate pred_rel(data->notified, 5);
    BOOST_CHECK(data->condition.timed_wait(lock, boost::posix_time::seconds(10), pred_rel));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred_rel());
    BOOST_CHECK_EQUAL(data->notified, 5);
    data->awoken++;
    data->condition.notify_one();
}

void do_test_condition_waits()
{
    condition_test_data data;

    boost::thread thread(bind(&condition_test_waits, &data));

    {
        boost::unique_lock<boost::mutex> lock(data.mutex);
        BOOST_CHECK(lock ? true : false);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 1)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 1);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 2)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 2);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 3)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 3);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 4)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 4);


        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 5)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 5);
    }

    thread.join();
    BOOST_CHECK_EQUAL(data.awoken, 5);
}

void test_condition_waits()
{
    // We should have already tested notify_one here, so
    // a timed test with the default execution_monitor::use_condition
    // should be OK, and gives the fastest performance
    timed_test(&do_test_condition_waits, 12);
}

void do_test_condition_wait_is_a_interruption_point()
{
    condition_test_data data;

    boost::thread thread(bind(&condition_test_thread, &data));

    thread.interrupt();
    thread.join();
    BOOST_CHECK_EQUAL(data.awoken,0);
}


void test_condition_wait_is_a_interruption_point()
{
    timed_test(&do_test_condition_wait_is_a_interruption_point, 1);
}

boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: condition test suite");

    test->add(BOOST_TEST_CASE(&test_condition_waits));
    test->add(BOOST_TEST_CASE(&test_condition_wait_is_a_interruption_point));

    return test;
}


