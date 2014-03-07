/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"
#include "../testfrmwk.hpp"

int
main() 
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  //These are a compile check / test.  They have to be hand inspected
  //b/c they depend on the TZ settings of the machine and hence it is
  //unclear what the results will be
  typedef boost::date_time::c_local_adjustor<ptime> local_adj;

  ptime t1(date(2002,Jan,1), hours(7)+millisec(5)); 
  std::cout << "UTC <--> TZ Setting of Machine -- No DST" << std::endl;
  ptime t2 = local_adj::utc_to_local(t1);
  std::cout << to_simple_string(t2) << " LOCAL is " 
            << to_simple_string(t1) << " UTC time "
            << std::endl;
  time_duration td1 = t2 - t1;
  std::cout << "A difference of: " << to_simple_string(td1)
            << std::endl;


  ptime t3(date(2002,May,1), hours(5)+millisec(5)); 
  std::cout << "UTC <--> TZ Setting of Machine -- In DST" << std::endl;
  ptime t4 = local_adj::utc_to_local(t3);
  std::cout << to_simple_string(t4) << " LOCAL is " 
            << to_simple_string(t3) << " UTC time "
            << std::endl;
  time_duration td2 = t4 - t3;
  std::cout << "A difference of: " << to_simple_string(td2)
            << std::endl;

  return printTestStats();
}

