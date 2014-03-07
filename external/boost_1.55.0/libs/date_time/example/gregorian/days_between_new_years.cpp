/* Provides a simple example of using a date_generator, and simple
 * mathematical operatorations, to calculate the days since 
 * New Years day of this year, and days until next New Years day.
 *
 * Expected results:
 * Adding together both durations will produce 365 (366 in a leap year).
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>

int
main() 
{
  
  using namespace boost::gregorian;

  date today = day_clock::local_day();
  partial_date new_years_day(1,Jan);
  //Subtract two dates to get a duration
  days days_since_year_start = today - new_years_day.get_date(today.year());
  std::cout << "Days since Jan 1: " << days_since_year_start.days() 
            << std::endl;
  
  days days_until_year_start = new_years_day.get_date(today.year()+1) - today;
  std::cout << "Days until next Jan 1: " << days_until_year_start.days() 
            << std::endl;
  return 0;
}


/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

