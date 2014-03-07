/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/greg_month.hpp"
#include "../testfrmwk.hpp"
#include <iostream>


void 
test_month() 
{
  using namespace boost::gregorian;
  greg_month m1(Jan), m2(Feb), m3(Mar), m4(Apr), m5(May), m6(Jun);
  greg_month m7(Jul), m8(Aug), m9(Sep), m10(Oct), m11(Nov),m12(Dec);
  check("January  as_number", m1.as_number() == 1);
  check("December as_number", m12.as_number() == 12);
  check("Jan as Short String",  m1.as_short_string()  == std::string("Jan"));
  check("Feb as Short String",  m2.as_short_string()  == std::string("Feb"));
  check("Mar as Short String",  m3.as_short_string()  == std::string("Mar"));
  check("Apr as Short String",  m4.as_short_string()  == std::string("Apr"));
  check("May as Short String",  m5.as_short_string()  == std::string("May"));
  check("Jun as Short String",  m6.as_short_string()  == std::string("Jun"));
  check("Jul as Short String",  m7.as_short_string()  == std::string("Jul"));
  check("Aug as Short String",  m8.as_short_string()  == std::string("Aug"));
  check("Sep as Short String",  m9.as_short_string()  == std::string("Sep"));
  check("Oct as Short String",  m10.as_short_string() == std::string("Oct"));
  check("Nov as Short String",  m11.as_short_string() == std::string("Nov"));
  check("Dec as Short String",  m12.as_short_string() == std::string("Dec"));
  check("Jan as Long String",  m1.as_long_string()  == std::string("January"));
  check("Feb as Long String",  m2.as_long_string()  == std::string("February"));
  check("Mar as Long String",  m3.as_long_string()  == std::string("March"));
  check("Apr as Long String",  m4.as_long_string()  == std::string("April"));
  check("May as Long String",  m5.as_long_string()  == std::string("May"));
  check("Jun as Long String",  m6.as_long_string()  == std::string("June"));
  check("Jul as Long String",  m7.as_long_string()  == std::string("July"));
  check("Aug as Long String",  m8.as_long_string()  == std::string("August"));
  check("Sep as Long String",  m9.as_long_string()  == std::string("September"));
  check("Oct as Long String",  m10.as_long_string() == std::string("October"));
  check("Nov as Long String",  m11.as_long_string() == std::string("November"));
  check("Dec as Long String",  m12.as_long_string() == std::string("December"));
  //month m(5);

  //TODO can this support NAM? or check exception
  //  greg_month sm0(0);
  greg_month sm1(1);
  greg_month sm12(12);
  //check("check not a month value", sm0.as_short_string() == "NAM");
  check("short construction -- 1", 
        sm1.as_short_string() == std::string("Jan"));
  check("short construction -- 12", 
        sm12.as_short_string() == std::string("Dec"));
  check("month traits min", (greg_month::min)() == 1);
  check("month traits max", (greg_month::max)() == 12);

}

int
main() 
{
  test_month();
  return printTestStats();
}

