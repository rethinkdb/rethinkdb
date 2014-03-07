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

// <boost/thread/condition_variable.hpp>

// void notify_all_at_thread_exit(condition_variable& cond, unique_lock<mutex> lk);

#define BOOST_THREAD_USES_MOVE
#define BOOST_THREAD_VESRION 3

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::condition_variable cv;
boost::mutex mut;

typedef boost::chrono::milliseconds ms;
typedef boost::chrono::high_resolution_clock Clock;

void func()
{
  boost::unique_lock < boost::mutex > lk(mut);
  boost::notify_all_at_thread_exit(cv, boost::move(lk));
  boost::this_thread::sleep_for(ms(300));
}

int main()
{
  boost::unique_lock < boost::mutex > lk(mut);
  boost::thread(func).detach();
  Clock::time_point t0 = Clock::now();
  cv.wait(lk);
  Clock::time_point t1 = Clock::now();
  BOOST_TEST(t1 - t0 > ms(250));
  return boost::report_errors();
}

