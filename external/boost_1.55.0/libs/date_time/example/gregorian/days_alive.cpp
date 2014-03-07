/* Short example that calculates the number of days since user was born.
 * Demonstrates comparisons of durations, use of the day_clock,
 * and parsing a date from a string.
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>

int
main() 
{
  
  using namespace boost::gregorian;
  std::string s;
  std::cout << "Enter birth day YYYY-MM-DD (eg: 2002-02-01): ";
  std::cin >> s;
  try {
    date birthday(from_simple_string(s));
    date today = day_clock::local_day();
    days days_alive = today - birthday;
    days one_day(1);
    if (days_alive == one_day) {
      std::cout << "Born yesterday, very funny" << std::endl;
    }
    else if (days_alive < days(0)) {
      std::cout << "Not born yet, hmm: " << days_alive.days() 
                << " days" <<std::endl;
    }
    else {
      std::cout << "Days alive: " << days_alive.days() << std::endl;
    }

  }
  catch(...) {
    std::cout << "Bad date entered: " << s << std::endl;
  }
  return 0;
}


/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

