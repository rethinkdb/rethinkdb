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

// <boost/thread/recursive_mutex.hpp>

// class recursive_timed_mutex;

// template <class Rep, class Period>
//     bool try_lock_for(const chrono::duration<Rep, Period>& rel_time);

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO


boost::recursive_timed_mutex m;

typedef boost::chrono::steady_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef boost::chrono::milliseconds ms;
typedef boost::chrono::nanoseconds ns;

void f1()
{
  time_point t0 = Clock::now();
  // This test is spurious as it depends on the time the thread system switches the threads
  BOOST_TEST(m.try_lock_for(ms(300)+ms(1000)) == true);
  time_point t1 = Clock::now();
  BOOST_TEST(m.try_lock());
  m.unlock();
  m.unlock();
  ns d = t1 - t0 - ms(250);
  // This test is spurious as it depends on the time the thread system switches the threads
  BOOST_TEST(d < ns(5000000)+ms(1000)); // within 5ms
}

void f2()
{
  time_point t0 = Clock::now();
  BOOST_TEST(m.try_lock_for(ms(250)) == false);
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
    boost::this_thread::sleep_for(ms(400));
    m.unlock();
    t.join();
  }

  return boost::report_errors();
}

#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif

