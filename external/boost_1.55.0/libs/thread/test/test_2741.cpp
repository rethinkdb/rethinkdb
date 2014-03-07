// Copyright (C) 2008 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread_only.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/utility.hpp>

#include <iostream>
#include <boost/test/unit_test.hpp>

#define DEFAULT_EXECUTION_MONITOR_TYPE execution_monitor::use_sleep_only
#include "./util.inl"

int test_value;
#ifdef PTHREAD_STACK_MIN
#define MY_PTHREAD_STACK PTHREAD_STACK_MIN
#else
#define MY_PTHREAD_STACK 4*0x4000
#endif
void simple_thread()
{
  test_value = 999;
}

void test_native_handle()
{

  boost::thread_attributes attrs;

  boost::thread_attributes::native_handle_type* h = attrs.native_handle();
#if defined(BOOST_THREAD_PLATFORM_WIN32)
  // ... window version
#elif defined(BOOST_THREAD_PLATFORM_PTHREAD)

  int k = pthread_attr_setstacksize(h, MY_PTHREAD_STACK);
  std::cout << k << std::endl;
  BOOST_CHECK(!pthread_attr_setstacksize(h, MY_PTHREAD_STACK));
  std::size_t res;
  BOOST_CHECK(!pthread_attr_getstacksize(h, &res));
  BOOST_CHECK(res >= (MY_PTHREAD_STACK));
#else
#error "Boost thread unavailable on this platform"
#endif

}

void test_stack_size()
{
  boost::thread_attributes attrs;

  attrs.set_stack_size(0x4000);
  BOOST_CHECK(attrs.get_stack_size() >= 0x4000);

}
void do_test_creation_with_attrs()
{
  test_value = 0;
  boost::thread_attributes attrs;
  attrs.set_stack_size(0x4000);
  boost::thread thrd(attrs, &simple_thread);
  thrd.join();
  BOOST_CHECK_EQUAL(test_value, 999);
}

void test_creation_with_attrs()
{
  timed_test(&do_test_creation_with_attrs, 1);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
  boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE("Boost.Threads: thread attributes test suite");

  test->add(BOOST_TEST_CASE(test_native_handle));
  test->add(BOOST_TEST_CASE(test_stack_size));
  test->add(BOOST_TEST_CASE(test_creation_with_attrs));

  return test;
}


