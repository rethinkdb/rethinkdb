//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class lock_guard;

// lock_guard(mutex_type& m, adopt_lock_t);

#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#ifdef BOOST_THREAD_USES_CHRONO
typedef boost::chrono::high_resolution_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef boost::chrono::milliseconds ms;
typedef boost::chrono::nanoseconds ns;
#endif
boost::mutex m;

void f()
{
#ifdef BOOST_THREAD_USES_CHRONO
  time_point t0 = Clock::now();
  time_point t1;
  {
    m.lock();
    boost::lock_guard<boost::mutex> lg(m, boost::adopt_lock);
    t1 = Clock::now();
  }
  ns d = t1 - t0 - ms(250);
  BOOST_TEST(d < ns(2500000)+ms(1000)); // within 2.5ms
#else
  //time_point t0 = Clock::now();
  //time_point t1;
  {
    m.lock();
    boost::lock_guard<boost::mutex> lg(m, boost::adopt_lock);
    //t1 = Clock::now();
  }
  //ns d = t1 - t0 - ms(250);
  //BOOST_TEST(d < ns(2500000)+ms(1000)); // within 2.5ms
#endif
}

int main()
{
  m.lock();
  boost::thread t(f);
#ifdef BOOST_THREAD_USES_CHRONO
  boost::this_thread::sleep_for(ms(250));
#endif
  m.unlock();
  t.join();

  return boost::report_errors();
}

