/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/greg_year.hpp"
#include "../testfrmwk.hpp"
#include <iostream>


int
main() 
{

  using namespace boost::gregorian;
  greg_year d1(1400);
  check("Basic of min", d1 == 1400);
  greg_year d2(10000);
  check("Basic test of max", d2 == 10000);
  try {
    greg_year bad(0);
    check("Bad year creation", false); //oh oh, fail
    //unreachable
    std::cout << "Shouldn't reach here: " << bad << std::endl;
  }
  catch(std::exception &) {
    check("Bad year creation", true); //good
    
  }
  try {
    greg_year bad(10001);
    check("Bad year creation2", false); //oh oh, fail
    //unreachable
    std::cout << "Shouldn't reach here: " << bad << std::endl;
  }
  catch(std::exception&) {
    check("Bad year creation2", true); //good
    
  }
  check("traits min year", (greg_year::min)() ==  1400);
  check("traits max year", (greg_year::max)() == 10000);

  printTestStats();
  return 0;
}

