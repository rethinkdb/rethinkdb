/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/greg_day.hpp"
#include "boost/date_time/gregorian/greg_weekday.hpp"
#include "boost/date_time/gregorian/greg_day_of_year.hpp"
#include "../testfrmwk.hpp"
#include <iostream>


void 
test_day() 
{
  using namespace boost::gregorian;
  greg_day d1(1);
  check("Basic test", d1 == 1);
  try {
    greg_day bad(0);
    check("Bad day creation", false); //oh oh, fail
    //unreachable
    std::cout << "Shouldn't reach here: " << bad << std::endl;
  }
  catch(std::exception &) {
    check("Bad day creation", true); //good
    
  }
  try {
    greg_day bad(32);
    check("Bad day creation2", false); //oh oh, fail
    //unreachable
    std::cout << "Shouldn't reach here: " << bad << std::endl;
  }
  catch(std::exception&) {
    check("Bad day creation2", true); //good
    
  }
  check("traits min day", (greg_day::min)() == 1);
  check("traits max day", (greg_day::max)() == 31);

  greg_weekday sunday(0);
  greg_weekday monday(1);

  check("Weekday 0 short name == Sun", 
        sunday.as_short_string() == std::string("Sun"));
  check("Weekday 1 short name == Mon", 
        monday.as_short_string() == std::string("Mon"));
  check("Weekday 2 short name == Tue", 
        greg_weekday(2).as_short_string() == std::string("Tue"));
  check("Weekday 3 short name == Wed", 
        greg_weekday(3).as_short_string() == std::string("Wed"));
  check("Weekday 4 short name == Thu", 
        greg_weekday(4).as_short_string() == std::string("Thu"));
  check("Weekday 5 short name == Fri", 
        greg_weekday(5).as_short_string() == std::string("Fri"));
  check("Weekday 6 short name == Sat", 
        greg_weekday(6).as_short_string() == std::string("Sat"));
  try {
    greg_weekday bad(7);
    check("Bad weekday creation", false); //oh oh, fail
    //unreachable
    std::cout << "Shouldn't reach here: " << bad << std::endl;
  }
  catch(bad_weekday&) {
    check("Bad weekday creation", true); //good
    
  }

   try {
     greg_day_of_year_rep bad(367);
     check("Bad day of year", false); //oh oh, fail
    //unreachable
    std::cout << "Shouldn't reach here: " << bad << std::endl;

   }
   catch(bad_day_of_year&) {
     check("Bad day of year", true); //good
  }
  
}

int
main() 
{
  test_day();
  return printTestStats();
}

