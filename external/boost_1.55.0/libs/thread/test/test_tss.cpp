// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2
#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/thread/detail/config.hpp>

#include <boost/thread/tss.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <boost/test/unit_test.hpp>

#include "./util.inl"

#include <iostream>

#if defined(BOOST_HAS_WINTHREADS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

boost::mutex check_mutex;
boost::mutex tss_mutex;
int tss_instances = 0;
int tss_total = 0;

struct tss_value_t
{
    tss_value_t()
    {
        boost::unique_lock<boost::mutex> lock(tss_mutex);
        ++tss_instances;
        ++tss_total;
        value = 0;
    }
    ~tss_value_t()
    {
        boost::unique_lock<boost::mutex> lock(tss_mutex);
        --tss_instances;
    }
    int value;
};

boost::thread_specific_ptr<tss_value_t> tss_value;

void test_tss_thread()
{
    tss_value.reset(new tss_value_t());
    for (int i=0; i<1000; ++i)
    {
        int& n = tss_value->value;
        // Don't call BOOST_CHECK_EQUAL directly, as it doesn't appear to
        // be thread safe. Must evaluate further.
        if (n != i)
        {
            boost::unique_lock<boost::mutex> lock(check_mutex);
            BOOST_CHECK_EQUAL(n, i);
        }
        ++n;
    }
}

#if defined(BOOST_THREAD_PLATFORM_WIN32)
    typedef HANDLE native_thread_t;

    DWORD WINAPI test_tss_thread_native(LPVOID /*lpParameter*/)
    {
        test_tss_thread();
        return 0;
    }

    native_thread_t create_native_thread(void)
    {
        native_thread_t const res=CreateThread(
            0, //security attributes (0 = not inheritable)
            0, //stack size (0 = default)
            &test_tss_thread_native, //function to execute
            0, //parameter to pass to function
            0, //creation flags (0 = run immediately)
            0  //thread id (0 = thread id not returned)
            );
        BOOST_CHECK(res!=0);
        return res;
    }

    void join_native_thread(native_thread_t thread)
    {
        DWORD res = WaitForSingleObject(thread, INFINITE);
        BOOST_CHECK(res == WAIT_OBJECT_0);

        res = CloseHandle(thread);
        BOOST_CHECK(SUCCEEDED(res));
    }
#elif defined(BOOST_THREAD_PLATFORM_PTHREAD)
    typedef pthread_t native_thread_t;

extern "C"
{
    void* test_tss_thread_native(void* )
    {
        test_tss_thread();
        return 0;
    }
}

    native_thread_t create_native_thread()
    {
        native_thread_t thread_handle;

        int const res = pthread_create(&thread_handle, 0, &test_tss_thread_native, 0);
        BOOST_CHECK(!res);
        return thread_handle;
    }

    void join_native_thread(native_thread_t thread)
    {
        void* result=0;
        int const res=pthread_join(thread,&result);
        BOOST_CHECK(!res);
    }
#endif

void do_test_tss()
{
    tss_instances = 0;
    tss_total = 0;

    const int NUMTHREADS=5;
    boost::thread_group threads;
    try
    {
        for (int i=0; i<NUMTHREADS; ++i)
            threads.create_thread(&test_tss_thread);
        threads.join_all();
    }
    catch(...)
    {
        threads.interrupt_all();
        threads.join_all();
        throw;
    }


    std::cout
        << "tss_instances = " << tss_instances
        << "; tss_total = " << tss_total
        << "\n";
    std::cout.flush();

    BOOST_CHECK_EQUAL(tss_instances, 0);
    BOOST_CHECK_EQUAL(tss_total, 5);

    tss_instances = 0;
    tss_total = 0;

    native_thread_t thread1 = create_native_thread();
    native_thread_t thread2 = create_native_thread();
    native_thread_t thread3 = create_native_thread();
    native_thread_t thread4 = create_native_thread();
    native_thread_t thread5 = create_native_thread();

    join_native_thread(thread5);
    join_native_thread(thread4);
    join_native_thread(thread3);
    join_native_thread(thread2);
    join_native_thread(thread1);

    std::cout
        << "tss_instances = " << tss_instances
        << "; tss_total = " << tss_total
        << "\n";
    std::cout.flush();

    // The following is not really an error. TSS cleanup support still is available for boost threads.
    // Also this usually will be triggered only when bound to the static version of thread lib.
    // 2006-10-02 Roland Schwarz
    //BOOST_CHECK_EQUAL(tss_instances, 0);
    BOOST_CHECK_MESSAGE(tss_instances == 0, "Support of automatic tss cleanup for native threading API not available");
    BOOST_CHECK_EQUAL(tss_total, 5);
}

void test_tss()
{
    timed_test(&do_test_tss, 2);
}

bool tss_cleanup_called=false;

struct Dummy
{};

void tss_custom_cleanup(Dummy* d)
{
    delete d;
    tss_cleanup_called=true;
}

boost::thread_specific_ptr<Dummy> tss_with_cleanup(tss_custom_cleanup);

void tss_thread_with_custom_cleanup()
{
    tss_with_cleanup.reset(new Dummy);
}

void do_test_tss_with_custom_cleanup()
{
    boost::thread t(tss_thread_with_custom_cleanup);
    try
    {
        t.join();
    }
    catch(...)
    {
        t.interrupt();
        t.join();
        throw;
    }

    BOOST_CHECK(tss_cleanup_called);
}


void test_tss_with_custom_cleanup()
{
    timed_test(&do_test_tss_with_custom_cleanup, 2);
}

Dummy* tss_object=new Dummy;

void tss_thread_with_custom_cleanup_and_release()
{
    tss_with_cleanup.reset(tss_object);
    tss_with_cleanup.release();
}

void do_test_tss_does_no_cleanup_after_release()
{
    tss_cleanup_called=false;
    boost::thread t(tss_thread_with_custom_cleanup_and_release);
    try
    {
        t.join();
    }
    catch(...)
    {
        t.interrupt();
        t.join();
        throw;
    }

    BOOST_CHECK(!tss_cleanup_called);
    if(!tss_cleanup_called)
    {
        delete tss_object;
    }
}

struct dummy_class_tracks_deletions
{
    static unsigned deletions;

    ~dummy_class_tracks_deletions()
    {
        ++deletions;
    }

};

unsigned dummy_class_tracks_deletions::deletions=0;

boost::thread_specific_ptr<dummy_class_tracks_deletions> tss_with_null_cleanup(NULL);

void tss_thread_with_null_cleanup(dummy_class_tracks_deletions* delete_tracker)
{
    tss_with_null_cleanup.reset(delete_tracker);
}

void do_test_tss_does_no_cleanup_with_null_cleanup_function()
{
    dummy_class_tracks_deletions* delete_tracker=new dummy_class_tracks_deletions;
    boost::thread t(tss_thread_with_null_cleanup,delete_tracker);
    try
    {
        t.join();
    }
    catch(...)
    {
        t.interrupt();
        t.join();
        throw;
    }

    BOOST_CHECK(!dummy_class_tracks_deletions::deletions);
    if(!dummy_class_tracks_deletions::deletions)
    {
        delete delete_tracker;
    }
}

void test_tss_does_no_cleanup_after_release()
{
    timed_test(&do_test_tss_does_no_cleanup_after_release, 2);
}

void test_tss_does_no_cleanup_with_null_cleanup_function()
{
    timed_test(&do_test_tss_does_no_cleanup_with_null_cleanup_function, 2);
}

void thread_with_local_tss_ptr()
{
    {
        boost::thread_specific_ptr<Dummy> local_tss(tss_custom_cleanup);

        local_tss.reset(new Dummy);
    }
    BOOST_CHECK(tss_cleanup_called);
    tss_cleanup_called=false;
}


void test_tss_does_not_call_cleanup_after_ptr_destroyed()
{
    boost::thread t(thread_with_local_tss_ptr);
    t.join();
    BOOST_CHECK(!tss_cleanup_called);
}

void test_tss_cleanup_not_called_for_null_pointer()
{
    boost::thread_specific_ptr<Dummy> local_tss(tss_custom_cleanup);
    local_tss.reset(new Dummy);
    tss_cleanup_called=false;
    local_tss.reset(0);
    BOOST_CHECK(tss_cleanup_called);
    tss_cleanup_called=false;
    local_tss.reset(new Dummy);
    BOOST_CHECK(!tss_cleanup_called);
}

void test_tss_at_the_same_adress()
{
  for(int i=0; i<2; i++)
  {
    boost::thread_specific_ptr<Dummy> local_tss(tss_custom_cleanup);
    local_tss.reset(new Dummy);
    tss_cleanup_called=false;
    BOOST_CHECK(tss_cleanup_called);
    tss_cleanup_called=false;
    BOOST_CHECK(!tss_cleanup_called);
  }
}


boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: tss test suite");

    test->add(BOOST_TEST_CASE(test_tss));
    test->add(BOOST_TEST_CASE(test_tss_with_custom_cleanup));
    test->add(BOOST_TEST_CASE(test_tss_does_no_cleanup_after_release));
    test->add(BOOST_TEST_CASE(test_tss_does_no_cleanup_with_null_cleanup_function));
    test->add(BOOST_TEST_CASE(test_tss_does_not_call_cleanup_after_ptr_destroyed));
    test->add(BOOST_TEST_CASE(test_tss_cleanup_not_called_for_null_pointer));

    return test;
}
