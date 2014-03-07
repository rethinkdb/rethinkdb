/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include <boost/config.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

int
main() 
{

  using namespace boost::posix_time;
  ptime tl = second_clock::local_time();
  std::cout << to_simple_string(tl) << std::endl;

  ptime tu = second_clock::universal_time();
  std::cout << to_simple_string(tu) << std::endl;

#if !defined(BOOST_WINDOWS) && defined(_POSIX_TIMERS)
  for (int i=0; i < 3; ++i) {
    ptime t2 = second_clock::local_time();
    std::cout << to_simple_string(t2) << std::endl;
    sleep(1);
  }
#endif

#ifdef BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK
   for (int i=0; i < 10; ++i) {
     //ptime t2 = high_precision_clock::local_time();
     ptime t2 = microsec_clock::local_time();
     std::cout << to_simple_string(t2) << std::endl;
   }
#endif // BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK

  return 0;

}

