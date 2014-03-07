/* Some simple examples of constructing and calculating with times
 * Returns:
 * [2002-Feb-01 00:00:00/2002-Feb-01 23:59:59.999999999] contains 2002-Feb-01 03:00:05
 * [2002-Feb-01 00:00:00/2002-Feb-01 23:59:59.999999999] intersected with
 * [2002-Feb-01 00:00:00/2002-Feb-01 03:00:04.999999999] is 
 * [2002-Feb-01 00:00:00/2002-Feb-01 03:00:04.999999999]
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

using namespace boost::posix_time;
using namespace boost::gregorian;

//Create a simple period class to contain all the times in a day
class day_period : public time_period
{
public:
  day_period(date d) : time_period(ptime(d),//midnight
                                   ptime(d,hours(24)))
  {}

};

int
main() 
{

  date d(2002,Feb,1); //an arbitrary date
  //a period that represents a day  
  day_period dp(d);
  ptime t(d, hours(3)+seconds(5)); //an arbitray time on that day
  if (dp.contains(t)) {
    std::cout << to_simple_string(dp) << " contains "
              << to_simple_string(t)  << std::endl;
  }
  //a period that represents part of the day
  time_period part_of_day(ptime(d, hours(0)), t);
  //intersect the 2 periods and print the results
  if (part_of_day.intersects(dp)) {
    time_period result = part_of_day.intersection(dp);
    std::cout << to_simple_string(dp) << " intersected with\n"
              << to_simple_string(part_of_day) << " is \n"
              << to_simple_string(result) << std::endl;
  }
    
  
  return 0;
}


/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

