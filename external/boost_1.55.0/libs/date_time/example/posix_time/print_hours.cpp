/* Print the remaining hours of the day
 * Uses the clock to get the local time 
 * Use an iterator to iterate over the remaining hours
 * Retrieve the date part from a time
 *
 * Expected Output something like:
 *
 * 2002-Mar-08 16:30:59
 * 2002-Mar-08 17:30:59
 * 2002-Mar-08 18:30:59
 * 2002-Mar-08 19:30:59
 * 2002-Mar-08 20:30:59
 * 2002-Mar-08 21:30:59
 * 2002-Mar-08 22:30:59
 * 2002-Mar-08 23:30:59
 * Time left till midnight: 07:29:01
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

int
main() 
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  //get the current time from the clock -- one second resolution
  ptime now = second_clock::local_time();
  //Get the date part out of the time
  date today = now.date();
  date tomorrow = today + days(1);
  ptime tomorrow_start(tomorrow); //midnight 

  //iterator adds by one hour
  time_iterator titr(now,hours(1)); 
  for (; titr < tomorrow_start; ++titr) {
    std::cout << to_simple_string(*titr) << std::endl;
  }
  
  time_duration remaining = tomorrow_start - now;
  std::cout << "Time left till midnight: " 
            << to_simple_string(remaining) << std::endl;
  return 0;
}

/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

