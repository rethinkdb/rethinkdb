//  Copyright (c) 2011 Helge Bahmann
//  Copyright (c) 2012 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// Attempt to determine whether the memory ordering/ fence operations
// work as expected:
// Let two threads race accessing multiple shared variables and
// verify that "observable" order of operations matches with the
// ordering constraints specified.
//
// We assume that "memory ordering violation" events are exponentially
// distributed, with unknown "average time between violations"
// (which is just the reciprocal of exp distribution parameter lambda).
// Use a "relaxed ordering" implementation that intentionally exhibits
// a (hopefully observable) such violation to compute the maximum-likelihood
// estimate for this time. From this, compute an estimate that covers the
// unknown value with 0.995 confidence (using chi square quantile).
//
// Use this estimate to pick a timeout for the race tests of the
// atomic implementations such that under the assumed distribution
// we get 0.995 probability to detect a race (if there is one).
//
// Overall this yields 0.995 * 0.995 > 0.99 confidence that the
// fences work as expected if this test program does not
// report an error.
#include <boost/atomic.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/thread.hpp>

// Two threads perform the following operations:
//
// thread # 1        thread # 2
// store(a, 1)       store(b, 1)
// read(a)           read(b)
// x = read(b)       y = read(a)
//
// Under relaxed memory ordering, the case (x, y) == (0, 0) is
// possible. Under sequential consistency, this case is impossible.
//
// This "problem" is reproducible on all platforms, even x86.
template<boost::memory_order store_order, boost::memory_order load_order>
class total_store_order_test {
public:
    total_store_order_test(void);

    void run(boost::posix_time::time_duration & timeout);
    bool detected_conflict(void) const { return detected_conflict_; }
private:
    void thread1fn(void);
    void thread2fn(void);
    void check_conflict(void);

    boost::atomic<int> a_;
    /* insert a bit of padding to push the two variables into
    different cache lines and increase the likelihood of detecting
    a conflict */
    char pad_[512];
    boost::atomic<int> b_;

    boost::barrier barrier_;

    int vrfya1_, vrfyb1_, vrfya2_, vrfyb2_;

    boost::atomic<bool> terminate_threads_;
    boost::atomic<int> termination_consensus_;

    bool detected_conflict_;
    boost::mutex m_;
    boost::condition_variable c_;
};

template<boost::memory_order store_order, boost::memory_order load_order>
total_store_order_test<store_order, load_order>::total_store_order_test(void)
    : a_(0), b_(0), barrier_(2),
    terminate_threads_(false), termination_consensus_(0),
    detected_conflict_(false)
{
}

template<boost::memory_order store_order, boost::memory_order load_order>
void
total_store_order_test<store_order, load_order>::run(boost::posix_time::time_duration & timeout)
{
    boost::system_time start = boost::get_system_time();
    boost::system_time end = start + timeout;

    boost::thread t1(boost::bind(&total_store_order_test::thread1fn, this));
    boost::thread t2(boost::bind(&total_store_order_test::thread2fn, this));

    {
        boost::mutex::scoped_lock guard(m_);
        while (boost::get_system_time() < end && !detected_conflict_)
            c_.timed_wait(guard, end);
    }

    terminate_threads_.store(true, boost::memory_order_relaxed);

    t2.join();
    t1.join();

    boost::posix_time::time_duration duration = boost::get_system_time() - start;
    if (duration < timeout)
        timeout = duration;
}

volatile int backoff_dummy;

template<boost::memory_order store_order, boost::memory_order load_order>
void
total_store_order_test<store_order, load_order>::thread1fn(void)
{
    for (;;) {
        a_.store(1, store_order);
        int a = a_.load(load_order);
        int b = b_.load(load_order);

        barrier_.wait();

        vrfya1_ = a;
        vrfyb1_ = b;

        barrier_.wait();

        check_conflict();

        /* both threads synchronize via barriers, so either
        both threads must exit here, or they must both do
        another round, otherwise one of them will wait forever */
        if (terminate_threads_.load(boost::memory_order_relaxed)) for (;;) {
            int tmp = termination_consensus_.fetch_or(1, boost::memory_order_relaxed);

            if (tmp == 3)
                return;
            if (tmp & 4)
                break;
        }

        termination_consensus_.fetch_xor(4, boost::memory_order_relaxed);

        unsigned int delay = rand() % 10000;
        a_.store(0, boost::memory_order_relaxed);

        barrier_.wait();

        while(delay--) { backoff_dummy = delay; }
    }
}

template<boost::memory_order store_order, boost::memory_order load_order>
void
total_store_order_test<store_order, load_order>::thread2fn(void)
{
    for (;;) {
        b_.store(1, store_order);
        int b = b_.load(load_order);
        int a = a_.load(load_order);

        barrier_.wait();

        vrfya2_ = a;
        vrfyb2_ = b;

        barrier_.wait();

        check_conflict();

        /* both threads synchronize via barriers, so either
        both threads must exit here, or they must both do
        another round, otherwise one of them will wait forever */
        if (terminate_threads_.load(boost::memory_order_relaxed)) for (;;) {
            int tmp = termination_consensus_.fetch_or(2, boost::memory_order_relaxed);

            if (tmp == 3)
                return;
            if (tmp & 4)
                break;
        }

        termination_consensus_.fetch_xor(4, boost::memory_order_relaxed);


        unsigned int delay = rand() % 10000;
        b_.store(0, boost::memory_order_relaxed);

        barrier_.wait();

        while(delay--) { backoff_dummy = delay; }
    }
}

template<boost::memory_order store_order, boost::memory_order load_order>
void
total_store_order_test<store_order, load_order>::check_conflict(void)
{
    if (vrfyb1_ == 0 && vrfya2_ == 0) {
        boost::mutex::scoped_lock guard(m_);
        detected_conflict_ = true;
        terminate_threads_.store(true, boost::memory_order_relaxed);
        c_.notify_all();
    }
}

void
test_seq_cst(void)
{
    double sum = 0.0;

    /* take 10 samples */
    for (size_t n = 0; n < 10; n++) {
        boost::posix_time::time_duration timeout(0, 0, 10);

        total_store_order_test<boost::memory_order_relaxed, boost::memory_order_relaxed> test;
        test.run(timeout);
        if (!test.detected_conflict()) {
            BOOST_WARN_MESSAGE(false, "Failed to detect order=seq_cst violation while ith order=relaxed -- intrinsic ordering too strong for this test");
            return;
        }

        std::cout << "seq_cst violation with order=relaxed after " << boost::posix_time::to_simple_string(timeout) << "\n";

        sum = sum + timeout.total_microseconds();
    }

    /* determine maximum likelihood estimate for average time between
    race observations */
    double avg_race_time_mle = (sum / 10);

    /* pick 0.995 confidence (7.44 = chi square 0.995 confidence) */
    double avg_race_time_995 = avg_race_time_mle * 2 * 10 / 7.44;

    /* 5.298 = 0.995 quantile of exponential distribution */
    boost::posix_time::time_duration timeout = boost::posix_time::microseconds((long)(5.298 * avg_race_time_995));

    std::cout << "run seq_cst for " << boost::posix_time::to_simple_string(timeout) << "\n";

    total_store_order_test<boost::memory_order_seq_cst, boost::memory_order_relaxed> test;
    test.run(timeout);

    BOOST_CHECK_MESSAGE(!test.detected_conflict(), "sequential consistency");
}

int test_main(int, char *[])
{
    test_seq_cst();

    return 0;
}
