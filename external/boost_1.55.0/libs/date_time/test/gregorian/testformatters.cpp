/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"

int
main() 
{

  boost::gregorian::date d1(2002,01,02);
  std::string ds1 = boost::gregorian::to_simple_string(d1);
  check("check string:     " + ds1, ds1 == "2002-Jan-02");
 
  std::string ids1(boost::gregorian::to_iso_string(d1));
  //  std::cout << boost::gregorian::to_iso_string(d1) << std::endl;
  check("check iso normal: " + ids1, ids1 == "20020102");

  std::string sds1 = boost::gregorian::to_sql_string(d1);
  check("check sql string: "+sds1, sds1 == "2002-01-02");

  boost::gregorian::date d2(2001,12,30);
  std::string ds2 = boost::gregorian::to_simple_string(d2);
  check("check string:     "+ds2, ds2 == "2001-Dec-30");
  std::string ids2 = boost::gregorian::to_iso_extended_string(d2);
  check("check iso extended string: "+ids2, ids2 == "2001-12-30");

  using namespace boost::gregorian;
  date d3(neg_infin);
  std::cout  << "|" << to_simple_string(d3) << "|" << std::endl;
  check("check negative infinity",     
        (to_simple_string(d3) == std::string("-infinity")));
  date d4(pos_infin);
  check("check positive infinity",     
        (to_simple_string(d4) == std::string("+infinity")));
  date d5(not_a_date_time);
  std::cout << to_simple_string(d5) << "|" << std::endl;
  check("check not a date",     
        (to_simple_string(d5) == std::string("not-a-date-time")));

  date_period p1(date(2000,Jan,1), date(2001,Jan,1));
  check("check period format",     
        (to_simple_string(p1) == std::string("[2000-Jan-01/2000-Dec-31]")));
  date_period p2(date(2000,Jan,1), date(pos_infin));
  check("check period format",     
        (to_simple_string(p2) == std::string("[2000-Jan-01/+infinity]")));
  std::cout << to_simple_string(p2) << std::endl;

  

  //  TODO enhance wchar support 
//   std::wstringstream wss;
//   wss << d3 << std::endl;
//   std::wcout << d3;
  return printTestStats();

}

