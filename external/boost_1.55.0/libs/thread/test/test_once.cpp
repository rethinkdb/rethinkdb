// (C) Copyright 2006-7 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2
#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>

#define LOG \
  if (false) {} else std::cout << std::endl << __FILE__ << "[" << __LINE__ << "]"

boost::once_flag flag=BOOST_ONCE_INIT;
int var_to_init=0;
boost::mutex m;

void initialize_variable()
{
    // ensure that if multiple threads get in here, they are serialized, so we can see the effect
    boost::unique_lock<boost::mutex> lock(m);
    ++var_to_init;
}


void call_once_thread()
{
    unsigned const loop_count=100;
    int my_once_value=0;
    for(unsigned i=0;i<loop_count;++i)
    {
        boost::call_once(flag, initialize_variable);
        my_once_value=var_to_init;
        if(my_once_value!=1)
        {
            break;
        }
    }
    boost::unique_lock<boost::mutex> lock(m);
    BOOST_CHECK_EQUAL(my_once_value, 1);
}

void test_call_once()
{
  LOG;

    unsigned const num_threads=20;
    boost::thread_group group;

    try
    {
        for(unsigned i=0;i<num_threads;++i)
        {
            group.create_thread(&call_once_thread);
        }
        group.join_all();
    }
    catch(...)
    {
        group.interrupt_all();
        group.join_all();
        throw;
    }

    BOOST_CHECK_EQUAL(var_to_init,1);
}

int var_to_init_with_functor=0;

struct increment_value
{
    int* value;
    explicit increment_value(int* value_):
        value(value_)
    {}

    void operator()() const
    {
        boost::unique_lock<boost::mutex> lock(m);
        ++(*value);
    }
};

void call_once_with_functor()
{
    unsigned const loop_count=100;
    int my_once_value=0;
    static boost::once_flag functor_flag=BOOST_ONCE_INIT;
    for(unsigned i=0;i<loop_count;++i)
    {
        boost::call_once(functor_flag, increment_value(&var_to_init_with_functor));
        my_once_value=var_to_init_with_functor;
        if(my_once_value!=1)
        {
            break;
        }
    }
    boost::unique_lock<boost::mutex> lock(m);
    BOOST_CHECK_EQUAL(my_once_value, 1);
}

void test_call_once_arbitrary_functor()
{
  LOG;

    unsigned const num_threads=20;
    boost::thread_group group;

    try
    {
        for(unsigned i=0;i<num_threads;++i)
        {
            group.create_thread(&call_once_with_functor);
        }
        group.join_all();
    }
    catch(...)
    {
        group.interrupt_all();
        group.join_all();
        throw;
    }

    BOOST_CHECK_EQUAL(var_to_init_with_functor,1);
}


struct throw_before_third_pass
{
    struct my_exception
    {};

    static unsigned pass_counter;

    void operator()() const
    {
        boost::unique_lock<boost::mutex> lock(m);
        ++pass_counter;
        if(pass_counter<3)
        {
            throw my_exception();
        }
    }
};

unsigned throw_before_third_pass::pass_counter=0;
unsigned exception_counter=0;

void call_once_with_exception()
{
    static boost::once_flag functor_flag=BOOST_ONCE_INIT;
    try
    {
        boost::call_once(functor_flag, throw_before_third_pass());
    }
    catch(throw_before_third_pass::my_exception)
    {
        boost::unique_lock<boost::mutex> lock(m);
        ++exception_counter;
    }
}

void test_call_once_retried_on_exception()
{
  LOG;
    unsigned const num_threads=20;
    boost::thread_group group;

    try
    {
        for(unsigned i=0;i<num_threads;++i)
        {
            group.create_thread(&call_once_with_exception);
        }
        group.join_all();
    }
    catch(...)
    {
        group.interrupt_all();
        group.join_all();
        throw;
    }

    BOOST_CHECK_EQUAL(throw_before_third_pass::pass_counter,3u);
    BOOST_CHECK_EQUAL(exception_counter,2u);
}


boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: call_once test suite");

    test->add(BOOST_TEST_CASE(test_call_once));
    test->add(BOOST_TEST_CASE(test_call_once_arbitrary_functor));
    test->add(BOOST_TEST_CASE(test_call_once_retried_on_exception));

    return test;
}


