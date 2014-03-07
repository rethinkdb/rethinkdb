/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>

int
main() 
{

  boost::gregorian::date d1(boost::gregorian::day_clock::local_day());
  std::cout << "Check the printed date by hand: "
            <<  boost::gregorian::to_iso_string(d1) << std::endl;

  using namespace boost::gregorian;
  date::ymd_type ymd = day_clock::local_day_ymd();
  std::cout << ymd.year << "-" 
            << ymd.month.as_long_string() << "-"
            << ymd.day << std::endl;

  date d2(day_clock::universal_day());
  std::cout << "Getting UTC date: "
            <<  to_iso_string(d2) << std::endl;

  date::ymd_type ymd2 = day_clock::universal_day_ymd();
  std::cout << ymd2.year << "-" 
            << ymd2.month.as_long_string() << "-"
            << ymd2.day << std::endl;

  return 0;

}

