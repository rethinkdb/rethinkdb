/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

//#include "date_time/testfrmwk.hpp"
#include <iostream>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/cstdint.hpp"

int
main()
{
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
  //skipping tests here due to lack of operator<< support in msvc6
  // TODO: this is a bit misleading: using STLport, this should work.
  std::cout << "Skipping tests on MSVC6" << std::endl;

#else

  std::cout << "int64_t  max:  "
            << (std::numeric_limits<boost::int64_t>::max)() 
            << std::endl;
  std::cout << "uint64_t max: "
            << (std::numeric_limits<boost::uint64_t>::max)() 
            << std::endl;


  boost::int64_t seconds_per_day = 60*60*24;
  boost::int64_t microsec_per_sec = 1000000;
  boost::int64_t microsec_per_day = seconds_per_day*microsec_per_sec;
  std::cout << "microsec per day: " 
            << microsec_per_day
            << std::endl;

  boost::uint64_t total_days = (std::numeric_limits<boost::int64_t>::max)() / microsec_per_day;

  std::cout << "Representable days: " 
            << total_days
            << std::endl;

  boost::int64_t approx_years = total_days / 366;

  std::cout << "Approximate years: " 
            << approx_years
            << std::endl;
  
  //getting day count
 // usec_count / (seconds_per_day*usec_per_sec);
  boost::int64_t day_count = 1000;
  boost::int64_t usec_count1000 = day_count*microsec_per_day + 999999;
  std::cout << "usec count at day 1000 + 999999: " 
            << usec_count1000
            << std::endl;
  
  boost::int64_t day_count_calc = usec_count1000 / microsec_per_day;
  std::cout << "calc day count at day 1000: " 
            << day_count_calc
            << std::endl;

  boost::int64_t remaining_usec_count = usec_count1000 % microsec_per_day;
  std::cout << "remaining usec count: " 
            << remaining_usec_count
            << std::endl;
  
  boost::int32_t day_count3M = 3000000;
  boost::int64_t usec_count3M = day_count3M*microsec_per_day + 999999;
  std::cout << "usec count at day 3M + 999999: " 
            << usec_count3M
            << std::endl;

  boost::int64_t day_count_calc3M = usec_count3M / microsec_per_day;
  std::cout << "calc day count at day 3M: " 
            << day_count_calc3M
            << std::endl;

  boost::int64_t remaining_usec_count3M = usec_count3M % microsec_per_day;
  std::cout << "remaining usec count 3M: " 
            << remaining_usec_count3M
            << std::endl;

#endif  

//   std::cout << "Days from: "
//          << to_simple_string(d1) << " to "
//          << to_simple_string(d2) << " = "
//          << day_count << std::endl; 


  //  printTestStats();
  return 0;
};

