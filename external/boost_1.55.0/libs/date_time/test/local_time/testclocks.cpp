/* Copyright (c) 2003-2004 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-02-27 12:00:24 -0800 (Wed, 27 Feb 2008) $
 */

#include "boost/date_time/local_time/local_time.hpp"
#include <iostream>

// The actual clocks are tested in posix_time/testclock.cpp. 
// These tests are to verify that the time zone is applied correctly

int
main()
{
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  using namespace boost::local_time;


  boost::shared_ptr<time_zone> az_tz(new posix_time_zone("MST-07"));
  boost::shared_ptr<time_zone> ny_tz(new posix_time_zone("EST-05EDT,M4.1.0,M10.5.0"));

  ptime tl = second_clock::local_time();
  std::cout << to_simple_string(tl) << std::endl;
  local_date_time ldt1 = local_sec_clock::local_time(az_tz);
  std::cout << ldt1.to_string() << std::endl;
  local_date_time ldt2 = local_sec_clock::local_time(ny_tz);
  std::cout << ldt2.to_string() << std::endl;
  
  tl = microsec_clock::local_time();
  std::cout << to_simple_string(tl) << std::endl;
  local_date_time ldt3 = local_microsec_clock::local_time(az_tz);
  std::cout << ldt3.to_string() << std::endl;
  local_date_time ldt4 = local_microsec_clock::local_time(ny_tz);
  std::cout << ldt4.to_string() << std::endl;

   return 0;
}
