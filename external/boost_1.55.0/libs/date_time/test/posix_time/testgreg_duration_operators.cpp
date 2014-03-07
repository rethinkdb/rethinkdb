/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "../testfrmwk.hpp"


int main(){

#if !defined(BOOST_DATE_TIME_OPTIONAL_GREGORIAN_TYPES)
  // do not set this test to return fail - 
  // this is not necessarily a compiler problem
  check("Optional gregorian types not selected - no tests run", true);
#else

  using namespace boost::gregorian;
  using namespace boost::posix_time;
      

  /*** months ***/
  {
    ptime p(date(2001, Oct, 31), hours(5));
    check("ptime + months", 
        ptime(date(2002, Feb, 28), hours(5)) == p + months(4));
    p += months(4);
    check("ptime += months", 
        ptime(date(2002, Feb, 28), hours(5)) == p);
  }
  {
    ptime p(date(2001, Oct, 31), hours(5));
    check("ptime - months", 
        ptime(date(2001, Apr, 30), hours(5)) == p - months(6));
    p -= months(6);
    check("ptime -= months", 
        ptime(date(2001, Apr, 30), hours(5)) == p);
  }

  /*** years ***/
  {
    ptime p(date(2001, Feb, 28), hours(5));
    check("ptime + years", 
        ptime(date(2004, Feb, 29), hours(5)) == p + years(3));
    p += years(3);
    check("ptime += years", 
        ptime(date(2004, Feb, 29), hours(5)) == p);
  }
  {
    ptime p(date(2000, Feb, 29), hours(5));
    check("ptime - years", 
        ptime(date(1998, Feb, 28), hours(5)) == p - years(2));
    p -= years(2);
    check("ptime -= years", 
        ptime(date(1998, Feb, 28), hours(5)) == p);
  }

  
  /*** weeks ***/
  // shouldn't need many tests, it is nothing more than a date_duration
  // so all date_duration tests should prove this class
  {
    ptime p(date(2001, Feb, 28), hours(5));
    check("ptime + weeks", 
        ptime(date(2001, Mar, 21), hours(5)) == p + weeks(3));
    p += weeks(3);
    check("ptime += weeks", 
        ptime(date(2001, Mar, 21), hours(5)) == p);
  }
  {
    ptime p(date(2001, Feb, 28), hours(5));
    check("ptime - weeks", 
        ptime(date(2001, Feb, 14), hours(5)) == p - weeks(2));
    p -= weeks(2);
    check("ptime -= weeks", 
        ptime(date(2001, Feb, 14), hours(5)) == p);
  }

#endif // BOOST_DATE_TIME_OPTIONAL_GREGORIAN_TYPES

  return printTestStats();
}
