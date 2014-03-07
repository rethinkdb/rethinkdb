//  Copyright (c) 2011 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// Attempt to determine whether the operations on atomic variables
// do in fact behave atomically: Let multiple threads race modifying
// a shared atomic variable and verify that it behaves as expected.
//
// We assume that "observable race condition" events are exponentially
// distributed, with unknown "average time between observable races"
// (which is just the reciprocal of exp distribution parameter lambda).
// Use a non-atomic implementation that intentionally exhibits a
// (hopefully tight) race to compute the maximum-likelihood estimate
// for this time. From this, compute an estimate that covers the
// unknown value with 0.995 confidence (using chi square quantile).
//
// Use this estimate to pick a timeout for the race tests of the
// atomic implementations such that under the assumed distribution
// we get 0.995 probability to detect a race (if there is one).
//
// Overall this yields 0.995 * 0.995 > 0.99 confidence that the
// operations truly behave atomic if this test program does not
// report an error.

#include <algorithm>

#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/thread.hpp>

/* helper class to let two instances of a function race against each
other, with configurable timeout and early abort on detection of error */
class concurrent_runner {
public:
    /* concurrently run the function in two threads, until either timeout
    or one of the functions returns "false"; returns true if timeout
    was reached, or false if early abort and updates timeout accordingly */
    static bool
    execute(
        const boost::function<bool(size_t)> & fn,
        boost::posix_time::time_duration & timeout)
    {
        concurrent_runner runner(fn);
        runner.wait_finish(timeout);
        return !runner.failure();
    }


    concurrent_runner(
        const boost::function<bool(size_t)> & fn)
        : finished_(false), failure_(false),
        first_thread_(boost::bind(&concurrent_runner::thread_function, this, fn, 0)),
        second_thread_(boost::bind(&concurrent_runner::thread_function, this, fn, 1))
    {
    }

    void
    wait_finish(boost::posix_time::time_duration & timeout)
    {
        boost::system_time start = boost::get_system_time();
        boost::system_time end = start + timeout;

        {
            boost::mutex::scoped_lock guard(m_);
            while (boost::get_system_time() < end && !finished())
                c_.timed_wait(guard, end);
        }

        finished_.store(true, boost::memory_order_relaxed);

        first_thread_.join();
        second_thread_.join();

        boost::posix_time::time_duration duration = boost::get_system_time() - start;
        if (duration < timeout)
            timeout = duration;
    }

    bool
    finished(void) const throw() {
        return finished_.load(boost::memory_order_relaxed);
    }

    bool
    failure(void) const throw() {
        return failure_;
    }
private:
    void
    thread_function(boost::function<bool(size_t)> function, size_t instance)
    {
        while (!finished()) {
            if (!function(instance)) {
                boost::mutex::scoped_lock guard(m_);
                failure_ = true;
                finished_.store(true, boost::memory_order_relaxed);
                c_.notify_all();
                break;
            }
        }
    }


    boost::mutex m_;
    boost::condition_variable c_;

    boost::atomic<bool> finished_;
    bool failure_;

    boost::thread first_thread_;
    boost::thread second_thread_;
};

bool
racy_add(volatile unsigned int & value, size_t instance)
{
    size_t shift = instance * 8;
    unsigned int mask = 0xff << shift;
    for (size_t n = 0; n < 255; n++) {
        unsigned int tmp = value;
        value = tmp + (1 << shift);

        if ((tmp & mask) != (n << shift))
            return false;
    }

    unsigned int tmp = value;
    value = tmp & ~mask;
    if ((tmp & mask) != mask)
        return false;

    return true;
}

/* compute estimate for average time between races being observable, in usecs */
static double
estimate_avg_race_time(void)
{
    double sum = 0.0;

    /* take 10 samples */
    for (size_t n = 0; n < 10; n++) {
        boost::posix_time::time_duration timeout(0, 0, 10);

        volatile unsigned int value(0);
        bool success = concurrent_runner::execute(
            boost::bind(racy_add, boost::ref(value), _1),
            timeout
        );

        if (success) {
            BOOST_FAIL("Failed to establish baseline time for reproducing race condition");
        }

        sum = sum + timeout.total_microseconds();
    }

    /* determine maximum likelihood estimate for average time between
    race observations */
    double avg_race_time_mle = (sum / 10);

    /* pick 0.995 confidence (7.44 = chi square 0.995 confidence) */
    double avg_race_time_995 = avg_race_time_mle * 2 * 10 / 7.44;

    return avg_race_time_995;
}

template<typename value_type, size_t shift_>
bool
test_arithmetic(boost::atomic<value_type> & shared_value, size_t instance)
{
    size_t shift = instance * 8;
    value_type mask = 0xff << shift;
    value_type increment = 1 << shift;

    value_type expected = 0;

    for (size_t n = 0; n < 255; n++) {
        value_type tmp = shared_value.fetch_add(increment, boost::memory_order_relaxed);
        if ( (tmp & mask) != (expected << shift) )
            return false;
        expected ++;
    }
    for (size_t n = 0; n < 255; n++) {
        value_type tmp = shared_value.fetch_sub(increment, boost::memory_order_relaxed);
        if ( (tmp & mask) != (expected << shift) )
            return false;
        expected --;
    }

    return true;
}

template<typename value_type, size_t shift_>
bool
test_bitops(boost::atomic<value_type> & shared_value, size_t instance)
{
    size_t shift = instance * 8;
    value_type mask = 0xff << shift;

    value_type expected = 0;

    for (size_t k = 0; k < 8; k++) {
        value_type mod = 1 << k;
        value_type tmp = shared_value.fetch_or(mod << shift, boost::memory_order_relaxed);
        if ( (tmp & mask) != (expected << shift))
            return false;
        expected = expected | mod;
    }
    for (size_t k = 0; k < 8; k++) {
        value_type tmp = shared_value.fetch_and( ~ (1 << (shift + k)), boost::memory_order_relaxed);
        if ( (tmp & mask) != (expected << shift))
            return false;
        expected = expected & ~(1<<k);
    }
    for (size_t k = 0; k < 8; k++) {
        value_type mod = 255 ^ (1 << k);
        value_type tmp = shared_value.fetch_xor(mod << shift, boost::memory_order_relaxed);
        if ( (tmp & mask) != (expected << shift))
            return false;
        expected = expected ^ mod;
    }

    value_type tmp = shared_value.fetch_and( ~mask, boost::memory_order_relaxed);
    if ( (tmp & mask) != (expected << shift) )
        return false;

    return true;
}

int test_main(int, char *[])
{
    boost::posix_time::time_duration reciprocal_lambda;

    double avg_race_time = estimate_avg_race_time();

    /* 5.298 = 0.995 quantile of exponential distribution */
    const boost::posix_time::time_duration timeout = boost::posix_time::microseconds((long)(5.298 * avg_race_time));

    {
        boost::atomic<unsigned int> value(0);

        /* testing two different operations in this loop, therefore
        enlarge timeout */
        boost::posix_time::time_duration tmp(timeout * 2);

        bool success = concurrent_runner::execute(
            boost::bind(test_arithmetic<unsigned int, 0>, boost::ref(value), _1),
            tmp
        );

        BOOST_CHECK_MESSAGE(success, "concurrent arithmetic");
    }

    {
        boost::atomic<unsigned int> value(0);

        /* testing three different operations in this loop, therefore
        enlarge timeout */
        boost::posix_time::time_duration tmp(timeout * 3);

        bool success = concurrent_runner::execute(
            boost::bind(test_bitops<unsigned int, 0>, boost::ref(value), _1),
            tmp
        );

        BOOST_CHECK_MESSAGE(success, "concurrent bitops");
    }
    return 0;
}
