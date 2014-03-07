/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include <iostream>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/time_period.hpp"
#include "../testfrmwk.hpp"

int main() 
{
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  date d1(2001,Jan, 1);
  ptime t1 (d1,hours(2));//2001-Jan-1 02:00:00
  ptime t2 (d1,hours(3));//2001-Jan-1 03:00:00
  time_period p1(t1,t2); //2001-Jan-1 02:59:59
  time_period p2(p1);
  check("copy construct & ==", p1 == p2);
  time_period p3 = p2;
  check("assignment", p3 == p2);
  time_period p4(t1,hours(1));
  check("length", p4.length() == hours(1));
  std::cout << to_simple_string(t1) << std::endl;
  std::cout << to_simple_string(p4) << std::endl;
  std::cout << to_simple_string(p1) << std::endl;
  check("construction and ==", p1 == p4);
  check("begin", p1.begin() == t1);
  check("last",  p1.end()   == t2);
  check("end",   p1.last()  == t2-time_duration::unit());
  
  std::cout << to_simple_string(p1) << std::endl;
  //  check("last",  p1.()  == t2);
  check("contains begin", p1.contains(t1));
  check("contains end-not", !p1.contains(t2));
  check("contains last",  p1.contains(t2-seconds(1)));
  ptime t3(date(2001,Jan,1),hours(4));
  time_period p5(t2,t3);
  check("check contains", !p1.contains(p5.begin()));
  check("check contains", !p5.contains(p1.begin()));
  check("operator== not equal case", !(p1 == p5));
  check("less than order", p1 < p5);
  check("greater than order", p5 > p1);
  check("not equal", p5 != p1);
  check("intersects with myself", p1.intersects(p1));
  check("not intersects", !(p1.intersects(p5)));
  check("not intersects", !(p5.intersects(p1)));

  time_period p6(p5);
  p6.shift(minutes(30));
  std::cout << to_simple_string(p5) << std::endl;
  std::cout << to_simple_string(p6) << std::endl;
  check("shifted intersects",    p5.intersects(p6));
  check("shifted intersects",    p6.intersects(p5));
  check("contains begin",        p5.contains(p6.begin()));
  p6.shift(minutes(30));
  std::cout << to_simple_string(p5) << std::endl;
  std::cout << to_simple_string(p6) << std::endl;
  check("shifted !intersects",    !p5.intersects(p6));
  check("shifted !intersects",    !p6.intersects(p5));
  p6.shift(minutes(-30));
  std::cout << to_simple_string(p5) << std::endl;
  std::cout << to_simple_string(p6) << std::endl;
  time_period p7 = p5.intersection(p6);
  std::cout << to_simple_string(p7) << std::endl;
  check("shifted intersection",    
        p7 == time_period(ptime(d1,time_duration(3,30,0)),
                          ptime(d1,time_duration(4,0,0))));
  
  
  return printTestStats();

}

