// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2008 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2
#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread_only.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/utility.hpp>

#include <boost/test/unit_test.hpp>

#define DEFAULT_EXECUTION_MONITOR_TYPE execution_monitor::use_sleep_only
#include "./util.inl"

int test_value;

void simple_thread()
{
    test_value = 999;
}

void comparison_thread(boost::thread::id parent)
{
    boost::thread::id const my_id=boost::this_thread::get_id();

    BOOST_CHECK(my_id != parent);
    boost::thread::id const my_id2=boost::this_thread::get_id();
    BOOST_CHECK(my_id == my_id2);

    boost::thread::id const no_thread_id=boost::thread::id();
    BOOST_CHECK(my_id != no_thread_id);
}

void test_sleep()
{
    boost::xtime xt = delay(3);
    boost::thread::sleep(xt);

    // Ensure it's in a range instead of checking actual equality due to time
    // lapse
    BOOST_CHECK(boost::threads::test::in_range(xt, 2));
}

void do_test_creation()
{
    test_value = 0;
    boost::thread thrd(&simple_thread);
    thrd.join();
    BOOST_CHECK_EQUAL(test_value, 999);
}

void test_creation()
{
    timed_test(&do_test_creation, 1);
}

void do_test_id_comparison()
{
    boost::thread::id const self=boost::this_thread::get_id();
    boost::thread thrd(boost::bind(&comparison_thread, self));
    thrd.join();
}

void test_id_comparison()
{
    timed_test(&do_test_id_comparison, 1);
}

void interruption_point_thread(boost::mutex* m,bool* failed)
{
    boost::unique_lock<boost::mutex> lk(*m);
    boost::this_thread::interruption_point();
    *failed=true;
}

void do_test_thread_interrupts_at_interruption_point()
{
    boost::mutex m;
    bool failed=false;
    boost::unique_lock<boost::mutex> lk(m);
    boost::thread thrd(boost::bind(&interruption_point_thread,&m,&failed));
    thrd.interrupt();
    lk.unlock();
    thrd.join();
    BOOST_CHECK(!failed);
}

void test_thread_interrupts_at_interruption_point()
{
    timed_test(&do_test_thread_interrupts_at_interruption_point, 1);
}

void disabled_interruption_point_thread(boost::mutex* m,bool* failed)
{
    boost::unique_lock<boost::mutex> lk(*m);
    boost::this_thread::disable_interruption dc;
    boost::this_thread::interruption_point();
    *failed=false;
}

void do_test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point()
{
    boost::mutex m;
    bool failed=true;
    boost::unique_lock<boost::mutex> lk(m);
    boost::thread thrd(boost::bind(&disabled_interruption_point_thread,&m,&failed));
    thrd.interrupt();
    lk.unlock();
    thrd.join();
    BOOST_CHECK(!failed);
}

void test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point()
{
    timed_test(&do_test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point, 1);
}

struct non_copyable_functor:
    boost::noncopyable
{
    unsigned value;

    non_copyable_functor(): boost::noncopyable(),
        value(0)
    {}

    void operator()()
    {
        value=999;
    }
};

void do_test_creation_through_reference_wrapper()
{
    non_copyable_functor f;

    boost::thread thrd(boost::ref(f));
    thrd.join();
    BOOST_CHECK_EQUAL(f.value, 999u);
}

void test_creation_through_reference_wrapper()
{
    timed_test(&do_test_creation_through_reference_wrapper, 1);
}

struct long_running_thread
{
    boost::condition_variable cond;
    boost::mutex mut;
    bool done;

    long_running_thread():
        done(false)
    {}

    void operator()()
    {
        boost::unique_lock<boost::mutex> lk(mut);
        while(!done)
        {
            cond.wait(lk);
        }
    }
};

void do_test_timed_join()
{
    long_running_thread f;
    boost::thread thrd(boost::ref(f));
    BOOST_CHECK(thrd.joinable());
    boost::system_time xt=delay(3);
    bool const joined=thrd.timed_join(xt);
    BOOST_CHECK(boost::threads::test::in_range(boost::get_xtime(xt), 2));
    BOOST_CHECK(!joined);
    BOOST_CHECK(thrd.joinable());
    {
        boost::unique_lock<boost::mutex> lk(f.mut);
        f.done=true;
        f.cond.notify_one();
    }

    xt=delay(3);
    bool const joined2=thrd.timed_join(xt);
    boost::system_time const now=boost::get_system_time();
    BOOST_CHECK(xt>now);
    BOOST_CHECK(joined2);
    BOOST_CHECK(!thrd.joinable());
}

void test_timed_join()
{
    timed_test(&do_test_timed_join, 10);
}

void test_swap()
{
    boost::thread t(simple_thread);
    boost::thread t2(simple_thread);
    boost::thread::id id1=t.get_id();
    boost::thread::id id2=t2.get_id();

    t.swap(t2);
    BOOST_CHECK(t.get_id()==id2);
    BOOST_CHECK(t2.get_id()==id1);

    swap(t,t2);
    BOOST_CHECK(t.get_id()==id1);
    BOOST_CHECK(t2.get_id()==id2);
}


boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread test suite");

    test->add(BOOST_TEST_CASE(test_sleep));
    test->add(BOOST_TEST_CASE(test_creation));
    test->add(BOOST_TEST_CASE(test_id_comparison));
    test->add(BOOST_TEST_CASE(test_thread_interrupts_at_interruption_point));
    test->add(BOOST_TEST_CASE(test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point));
    test->add(BOOST_TEST_CASE(test_creation_through_reference_wrapper));
    test->add(BOOST_TEST_CASE(test_timed_join));
    test->add(BOOST_TEST_CASE(test_swap));

    return test;
}

