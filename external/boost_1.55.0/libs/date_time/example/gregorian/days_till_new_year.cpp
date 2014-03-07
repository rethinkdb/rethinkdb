// This example displays the amount of time until new year's in days

#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>

int
main() 
{
  
  using namespace boost::gregorian;

  date::ymd_type today = day_clock::local_day_ymd();
  date new_years_day(today.year + 1,1,1);
  date_duration dd = new_years_day - date(today);
  
  std::cout << "Days till new year: " << dd.days() << std::endl;
  return 0;
}

/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

