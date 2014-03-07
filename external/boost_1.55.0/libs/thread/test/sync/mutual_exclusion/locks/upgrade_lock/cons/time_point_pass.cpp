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

// <boost/thread/locks.hpp>

// template <class Mutex> class upgrade_lock;

// template <class Clock, class Duration>
//   upgrade_lock(mutex_type& m, const chrono::time_point<Clock, Duration>& abs_time);

#define BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN

#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::shared_mutex m;

typedef boost::chrono::steady_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef boost::chrono::milliseconds ms;
typedef boost::chrono::nanoseconds ns;

void f1()
{
  time_point t0 = Clock::now();
  // This test is spurious as it depends on the time the thread system switches the threads
  boost::upgrade_lock<boost::shared_mutex> lk(m, Clock::now() + ms(300)+ms(1000));
  BOOST_TEST(lk.owns_lock() == true);
  time_point t1 = Clock::now();
  ns d = t1 - t0 - ms(250);
  // This test is spurious as it depends on the time the thread system switches the threads
  BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
}

void f2()
{
  time_point t0 = Clock::now();
  boost::upgrade_lock<boost::shared_mutex> lk(m, Clock::now() + ms(250));
  BOOST_TEST(lk.owns_lock() == false);
  time_point t1 = Clock::now();
  ns d = t1 - t0 - ms(250);
  // This test is spurious as it depends on the time the thread system switches the threads
  BOOST_TEST(d < ns(5000000)+ms(1000)); // within 5ms
}

int main()
{
  {
    m.lock();
    boost::thread t(f1);
    boost::this_thread::sleep_for(ms(250));
    m.unlock();
    t.join();
  }
  {
    m.lock();
    boost::thread t(f2);
    boost::this_thread::sleep_for(ms(300));
    m.unlock();
    t.join();
  }

  return boost::report_errors();
}

