//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/condition_variable>

// class condition_variable;

// void wait(unique_lock<mutex>& lock);

#include <iostream>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

boost::condition_variable cv;
boost::mutex mut;

int test1 = 0;
int test2 = 0;

int runs = 0;

void f()
{
  boost::unique_lock<boost::mutex> lk(mut);
  BOOST_TEST(test2 == 0);
  test1 = 1;
  cv.notify_one();
  while (test2 == 0) {
      cv.wait(lk);
  }
  BOOST_TEST(test2 != 0);
}

int main()
{
  boost::unique_lock<boost::mutex>lk(mut);
  boost::thread t(f);
  BOOST_TEST(test1 == 0);
  while (test1 == 0)
  {
      cv.wait(lk);
  }
  BOOST_TEST(test1 != 0);
  test2 = 1;
  lk.unlock();
  cv.notify_one();
  t.join();

  return boost::report_errors();
}
#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif

