// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class strict_lock;

// bool owns_lock(Mutex *) const;

#include <boost/thread/strict_lock.hpp>
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

int main()
{
  boost::mutex m;
  boost::mutex m2;

  boost::strict_lock<boost::mutex> lk(m);
  BOOST_TEST(lk.owns_lock(&m) == true);
  BOOST_TEST(!lk.owns_lock(&m2) == true);
  return boost::report_errors();
}
