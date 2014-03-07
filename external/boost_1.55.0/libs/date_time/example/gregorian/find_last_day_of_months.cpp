/* Simple program that finds the last day of the given month, 
 * then displays the last day of every month left in the given year.
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>

int
main()
{
  using namespace boost::gregorian;
  
  greg_year year(1400);
  greg_month month(1);

  // get a month and a year from the user
  try {
    int y, m;
    std::cout << "   Enter Year(ex: 2002): ";
    std::cin >> y;
    year = greg_year(y);
    std::cout << "   Enter Month(1..12): ";
    std::cin >> m;
    month = greg_month(m);
  }
  catch(bad_year by) {
    std::cout << "Invalid Year Entered: " << by.what() << '\n'
      << "Using minimum values for month and year." << std::endl;
  }
  catch(bad_month bm) {
    std::cout << "Invalid Month Entered" << bm.what() << '\n'
      << "Using minimum value for month. " << std::endl;
  }
  
  date start_of_next_year(year+1, Jan, 1);
  date d(year, month, 1);
  
  // add another month to d until we enter the next year.
  while (d < start_of_next_year){
    std::cout << to_simple_string(d.end_of_month()) << std::endl;
    d += months(1);
  }

  return 0;
}

/*  Copyright 2001-2005: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

