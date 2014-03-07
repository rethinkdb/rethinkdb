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

// <boost/thread/thread.hpp>

// thread::id this_thread::get_id();

#include <boost/thread/thread_only.hpp>
#include <cstdlib>
#include <algorithm>

#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

int main()
{
  typedef boost::chrono::system_clock Clock;
  typedef Clock::time_point time_point;
  boost::chrono::milliseconds ms(500);
  time_point t0 = Clock::now();
  boost::this_thread::sleep_for(ms);
  time_point t1 = Clock::now();
  boost::chrono::nanoseconds ns = (t1 - t0) - ms;
  boost::chrono::nanoseconds err = ms / 100;
  // The time slept is within 1% of 500ms
  // This test is spurious as it depends on the time the thread system switches the threads
  BOOST_TEST((std::max)(ns.count(), -ns.count()) < (err+boost::chrono::milliseconds(1000)).count());
  //BOOST_TEST(std::abs(static_cast<long>(ns.count())) < (err+boost::chrono::milliseconds(1000)).count());
  return boost::report_errors();

}

#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif

