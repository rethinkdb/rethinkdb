/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */

#include <iostream>
#include "boost/date_time/local_time/local_time.hpp"
#include "../testfrmwk.hpp"
// #if defined(BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS)
// #include "boost/date_time/gregorian/formatters_limited.hpp"
// #else
// #include "boost/date_time/gregorian/formatters.hpp"
// #endif

using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace boost::local_time;

void iterate_backward(const local_date_time *answers, 
                      int ary_len, 
                      const time_duration& td)
{
  int i = ary_len -1;
  local_date_time end = answers[i];
  local_time_iterator titr(end,td);
  
  std::cout << "counting down by previous duration..." << std::endl;
  for (; titr >= answers[0]; --titr) {
    std::cout << *titr << std::endl;
    check("iterating backward", answers[i] == *titr);
    --i;
  }
  check("iterating backward count", i == -1); // check the number of iterations
  std::cout << std::endl;
}

int
main() 
{

  time_zone_ptr ny_tz(new posix_time_zone("EST-05EDT,M4.1.0,M10.5.0"));

  //set up a time right on the dst boundary -- iterator will 
  //jump forward an hour at the boundary
  date d(2005,Apr,3);
  time_duration dst_offset(1,59,59);
  local_date_time start(d, dst_offset, ny_tz, false);

  const local_date_time answer1[] = 
    { 
      local_date_time(d,dst_offset, ny_tz, false), 
      local_date_time(d,hours(3), ny_tz, true),
      local_date_time(d,hours(3)+seconds(1), ny_tz, true), 
      local_date_time(d,hours(3)+seconds(2), ny_tz, true)
    };

  int i=0;
  local_time_iterator titr(start,seconds(1)); 
  local_date_time end = start + seconds(4);
  for (; titr < end; ++titr) {
    std::cout << (*titr) << std::endl;
    check("iterator -- 1 sec", answer1[i] == *titr);
    i++;
  }
  check("iterator -- 1 sec -- num iterations", i == 4); // check the number of iterations

  iterate_backward(answer1, 4, seconds(1));

  //iterate by hours
  const local_date_time answer2[] = 
    { local_date_time(d,dst_offset, ny_tz, false), 
      local_date_time(d,dst_offset+hours(2), ny_tz, true),
      local_date_time(d,dst_offset+hours(3), ny_tz, true), 
      local_date_time(d,dst_offset+hours(4), ny_tz, true)
    };
   i=0;
   local_time_iterator titr2(start,hours(1)); 
   local_date_time end2 = start + hours(4);
   for (; titr2 < end2; ++titr2) {
     std::cout << *titr2 << std::endl;
     check("iterator -- 1 hour", answer2[i] == *titr2);
     i++;
   }
   check("iterator -- 1 hour -- num iterations", i == 4); 

   iterate_backward(answer2, 4, hours(1));


 
  return printTestStats();
}

