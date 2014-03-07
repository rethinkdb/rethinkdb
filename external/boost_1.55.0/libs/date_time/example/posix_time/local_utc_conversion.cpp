/* Demonstrate conversions between a local time and utc
 * Output:
 * 
 * UTC <--> New York while DST is NOT active (5 hours)
 * 2001-Dec-31 19:00:00 in New York is 2002-Jan-01 00:00:00 UTC time 
 * 2002-Jan-01 00:00:00 UTC is 2001-Dec-31 19:00:00 New York time 
 * 
 * UTC <--> New York while DST is active (4 hours)
 * 2002-May-31 20:00:00 in New York is 2002-Jun-01 00:00:00 UTC time 
 * 2002-Jun-01 00:00:00 UTC is 2002-May-31 20:00:00 New York time 
 * 
 * UTC <--> Arizona (7 hours)
 * 2002-May-31 17:00:00 in Arizona is 2002-Jun-01 00:00:00 UTC time 
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time_adjustor.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"
#include <iostream>

int
main() 
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  //This local adjustor depends on the machine TZ settings-- highly dangerous!
  typedef boost::date_time::c_local_adjustor<ptime> local_adj;
  ptime t10(date(2002,Jan,1), hours(7)); 
  ptime t11 = local_adj::utc_to_local(t10);
  std::cout << "UTC <--> Zone base on TZ setting" << std::endl;
  std::cout << to_simple_string(t11) << " in your TZ is " 
            << to_simple_string(t10) << " UTC time "
            << std::endl;
  time_duration td = t11 - t10;
  std::cout << "A difference of: " 
            << to_simple_string(td) << std::endl;


  //eastern timezone is utc-5
  typedef boost::date_time::local_adjustor<ptime, -5, us_dst> us_eastern;

  ptime t1(date(2001,Dec,31), hours(19)); //5 hours b/f midnight NY time

  std::cout << "\nUTC <--> New York while DST is NOT active (5 hours)" 
            << std::endl;
  ptime t2 =  us_eastern::local_to_utc(t1);
  std::cout << to_simple_string(t1) << " in New York is " 
            << to_simple_string(t2) << " UTC time "
            << std::endl;

  ptime t3 = us_eastern::utc_to_local(t2);//back should be the same
  std::cout << to_simple_string(t2) << " UTC is " 
            << to_simple_string(t3) << " New York time "
            << "\n\n";

  ptime t4(date(2002,May,31), hours(20)); //4 hours b/f midnight NY time
  std::cout << "UTC <--> New York while DST is active (4 hours)" << std::endl;
  ptime t5 = us_eastern::local_to_utc(t4);
  std::cout << to_simple_string(t4) << " in New York is " 
            << to_simple_string(t5) << " UTC time "
            << std::endl;

  ptime t6 = us_eastern::utc_to_local(t5);//back should be the same
  std::cout << to_simple_string(t5) << " UTC is " 
            << to_simple_string(t6) << " New York time "
            << "\n" << std::endl;

    
  //Arizona timezone is utc-7 with no dst
  typedef boost::date_time::local_adjustor<ptime, -7, no_dst> us_arizona;

  ptime t7(date(2002,May,31), hours(17)); 
  std::cout << "UTC <--> Arizona (7 hours)" << std::endl;
  ptime t8 = us_arizona::local_to_utc(t7);
  std::cout << to_simple_string(t7) << " in Arizona is " 
            << to_simple_string(t8) << " UTC time "
            << std::endl;

  return 0;
}


/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

