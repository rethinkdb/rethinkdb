// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/thread_only.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <assert.h>
#include <iostream>
#include <stdlib.h>
#if defined(BOOST_THREAD_PLATFORM_PTHREAD)
#include <unistd.h>
#endif

boost::mutex mtx;
boost::condition_variable cv;

using namespace boost::posix_time;
using namespace boost::gregorian;
int main()
{
#if defined(BOOST_THREAD_PLATFORM_PTHREAD)

  for (int i=0; i<3; ++i)
  {
    const time_t now_time = ::time(0);
    const time_t wait_time = now_time+1;
    time_t end_time;
    assert(now_time < wait_time);

    boost::unique_lock<boost::mutex> lk(mtx);
    //const bool res =
    (void)cv.timed_wait(lk, from_time_t(wait_time));
    end_time = ::time(0);
    std::cerr << "now_time =" << now_time << " \n";
    std::cerr << "end_time =" << end_time << " \n";
    std::cerr << "wait_time=" << wait_time << " \n";
    std::cerr << "now_time =" << from_time_t(now_time) << " \n";
    std::cerr << "end_time =" << from_time_t(end_time) << " \n";
    std::cerr << "wait_time=" << from_time_t(wait_time) << " \n";
    std::cerr << end_time - wait_time << " \n";
    assert(end_time >= wait_time);
    std::cerr << " OK\n";
  }
#endif
  return 0;
}
