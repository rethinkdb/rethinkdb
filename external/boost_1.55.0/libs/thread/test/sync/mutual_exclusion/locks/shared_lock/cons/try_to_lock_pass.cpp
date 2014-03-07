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

// template <class Mutex> class shared_lock;

// shared_lock(mutex_type& m, try_to_lock_t);


#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::shared_mutex m;

#if defined BOOST_THREAD_USES_CHRONO
typedef boost::chrono::system_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef boost::chrono::milliseconds ms;
typedef boost::chrono::nanoseconds ns;
#else
#endif

void f()
{
#if defined BOOST_THREAD_USES_CHRONO
  time_point t0 = Clock::now();
  {
    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
  }
  {
    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
  }
  {
    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
  }
  while (true)
  {
    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
    if (lk.owns_lock()) break;
  }
  time_point t1 = Clock::now();
  ns d = t1 - t0 - ms(250);
  // This test is spurious as it depends on the time the thread system switches the threads
  BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
#else
//  time_point t0 = Clock::now();
//  {
//    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
//    BOOST_TEST(lk.owns_lock() == false);
//  }
//  {
//    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
//    BOOST_TEST(lk.owns_lock() == false);
//  }
//  {
//    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
//    BOOST_TEST(lk.owns_lock() == false);
//  }
  while (true)
  {
    boost::shared_lock<boost::shared_mutex> lk(m, boost::try_to_lock);
    if (lk.owns_lock()) break;
  }
  //time_point t1 = Clock::now();
  //ns d = t1 - t0 - ms(250);
  // This test is spurious as it depends on the time the thread system switches the threads
  //BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
#endif
}

int main()
{
  m.lock();
  boost::thread t(f);
#if defined BOOST_THREAD_USES_CHRONO
  boost::this_thread::sleep_for(ms(250));
#else
#endif
  m.unlock();
  t.join();

  return boost::report_errors();
}

