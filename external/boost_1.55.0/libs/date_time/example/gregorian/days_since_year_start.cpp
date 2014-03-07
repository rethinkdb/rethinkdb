
#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>

int
main() 
{
  
  using namespace boost::gregorian;

  date today = day_clock::local_day();
  //Subtract two dates to get a duration
  date_duration days_since_year_start = today - date(today.year(),Jan,1);
  std::cout << "Days since Jan 1: " << days_since_year_start.days() 
            << std::endl;
  return 0;
}

/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

