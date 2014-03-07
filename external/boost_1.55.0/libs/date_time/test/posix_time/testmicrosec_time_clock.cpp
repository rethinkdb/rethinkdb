/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/microsec_time_clock.hpp"
#include "../testfrmwk.hpp"
#if defined(BOOST_HAS_FTIME)
#include <windows.h>
#endif

int
main() 
{
#ifdef BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK

  using namespace boost::posix_time;

  //  ptime last = boost::date_time::microsec_resolution_clock<ptime>::local_time();
  ptime last = microsec_clock::local_time();

  int max = 30;
  int i = 0;
  for (i = 0; i<max; i++)
  {
    for (int j=0; j<100000; j++)
    {
      // some systems loop too fast so "last is less" tests fail
      // due to 'last' & 't2' being equal. These calls slow
      // it down enough to make 'last' & 't2' different
#if defined(BOOST_HAS_GETTIMEOFDAY)
      timeval tv;
      gettimeofday(&tv, 0);
#endif
#if defined(BOOST_HAS_FTIME)
      SYSTEMTIME st;
      GetSystemTime(&st);
#endif
    }

    ptime t1 = second_clock::local_time();
    std::cout << to_simple_string(t1) << std::endl;

    ptime t2 = microsec_clock::local_time();
    std::cout << to_simple_string(t2) << std::endl;
    check("hours match", t1.time_of_day().hours() == t2.time_of_day().hours());
    check("minutes match", 
          t1.time_of_day().minutes() == t2.time_of_day().minutes());
    check("seconds match", 
          t1.time_of_day().minutes() == t2.time_of_day().minutes());
    check("hours date", t1.date() == t2.date());
    if( !check("last is less", last <= t2) ) {
      std::cout << to_simple_string(last) << " < " 
        << to_simple_string(t2) << std::endl;
    }
    last = t2;

    
  }


  std::cout << "Now do the same test for universal time -- a few less iterations" << std::endl;
  max = 10;
  last = microsec_clock::universal_time();
  for (i = 0; i<max; i++)
  {
    for (int j=0; j<100000; j++)
    {
      // some systems loop too fast so "last is less" tests fail
      // due to 'last' & 't2' being equal. These calls slow
      // it down enough to make 'last' & 't2' different
#if defined(BOOST_HAS_GETTIMEOFDAY)
      timeval tv;
      gettimeofday(&tv, 0);
#endif
#if defined(BOOST_HAS_FTIME)
      SYSTEMTIME st;
      GetSystemTime(&st);
#endif
    }

    ptime t1 = second_clock::universal_time();
    std::cout << to_simple_string(t1) << std::endl;

    ptime t2 = microsec_clock::universal_time();
    std::cout << to_simple_string(t2) << std::endl;
    check("hours match", t1.time_of_day().hours() == t2.time_of_day().hours());
    check("minutes match", 
          t1.time_of_day().minutes() == t2.time_of_day().minutes());
    check("seconds match", 
          t1.time_of_day().minutes() == t2.time_of_day().minutes());
    check("hours date", t1.date() == t2.date());
    //following check might be equal on a really fast machine
    if( !check("last is less", last <= t2) ) {
      std::cout << to_simple_string(last) << " < " 
        << to_simple_string(t2) << std::endl;
    }
    last = t2;

    
  }

#else
  check("Get time of day micro second clock not supported due to inadequate compiler/platform", false);
#endif
  return printTestStats();

}

