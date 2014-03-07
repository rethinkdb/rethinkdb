/* Copyright (c) 2003-2004 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */


#include "boost/date_time/gregorian/gregorian.hpp"
//#include "boost/date_time/local_time/time_zone.hpp"
#include "../testfrmwk.hpp"

#include "boost/date_time/local_time/posix_time_zone.hpp"

#include <string>
#include <iostream>

int main(){
  using namespace boost::local_time;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  typedef posix_time_zone_base<wchar_t> w_posix_time_zone;
  
  std::wstring specs[] = {L"MST-07", L"MST-07:00:00",L"EST-05EDT,M4.1.0,M10.5.0", L"EST-05:00:00EDT+01:00:00,M4.1.0/02:00:00,M10.5.0/02:00:00",L"PST-08PDT,J46/1:30,J310",L"PST-08PDT,45,310/0:30:00"};

  w_posix_time_zone nyc1(specs[2]);
  w_posix_time_zone nyc2(specs[3]);
  time_duration td = hours(-5);

  check("Has DST", nyc1.has_dst() && nyc2.has_dst());
  check("UTC offset", nyc1.base_utc_offset() == td);
  check("UTC offsets match", nyc1.base_utc_offset() == nyc2.base_utc_offset());
  check("Abbrevs", nyc1.std_zone_abbrev() == std::wstring(L"EST"));
  check("Abbrevs", nyc2.std_zone_abbrev() == std::wstring(L"EST"));
  check("Abbrevs", nyc1.dst_zone_abbrev() == std::wstring(L"EDT"));
  check("Abbrevs", nyc2.dst_zone_abbrev() == std::wstring(L"EDT"));
  // names not available for w_posix_time_zone, abbrevs used in their place
  check("Names", nyc1.std_zone_name() == std::wstring(L"EST"));
  check("Names", nyc2.std_zone_name() == std::wstring(L"EST"));
  check("Names", nyc1.dst_zone_name() == std::wstring(L"EDT"));
  check("Names", nyc2.dst_zone_name() == std::wstring(L"EDT"));
  td = hours(1);
  check("dst offset", nyc1.dst_offset() == td);
  check("dst offsets match", nyc1.dst_offset() == nyc2.dst_offset());
  check("dst start", nyc1.dst_local_start_time(2003) == 
      ptime(date(2003,Apr,6),time_duration(2,0,0)));
  check("dst starts match", nyc1.dst_local_start_time(2003) == 
      nyc2.dst_local_start_time(2003)); 
  check("dst end", nyc1.dst_local_end_time(2003) == 
      ptime(date(2003,Oct,26),time_duration(2,0,0)));
  check("dst ends match", nyc1.dst_local_end_time(2003) == 
      nyc2.dst_local_end_time(2003)); 
  check("to posix string", 
        nyc1.to_posix_string() == std::wstring(L"EST-05EDT+01,M4.1.0/02:00,M10.5.0/02:00"));
  check("to posix string", 
        nyc2.to_posix_string() == std::wstring(L"EST-05EDT+01,M4.1.0/02:00,M10.5.0/02:00"));
  

  w_posix_time_zone az1(specs[0]);
  w_posix_time_zone az2(specs[1]);
  td = hours(-7);

  check("Has DST", !az1.has_dst() && !az2.has_dst());
  check("UTC offset", az1.base_utc_offset() == td);
  check("UTC offsets match", az1.base_utc_offset() == az2.base_utc_offset());
  check("dst start in non-dst zone", 
        az1.dst_local_start_time(2005) == ptime(not_a_date_time));
  check("dst end in non-dst zone", 
        az2.dst_local_end_time(2005) == ptime(not_a_date_time));
  check("Abbrevs", az1.std_zone_abbrev() == std::wstring(L"MST"));
  check("Abbrevs", az2.std_zone_abbrev() == std::wstring(L"MST"));
  // non-dst zones default to empty strings for dst names & abbrevs
  check("Abbrevs", az1.dst_zone_abbrev() == std::wstring(L""));
  check("Abbrevs", az2.dst_zone_abbrev() == std::wstring(L""));
  check("Names", az1.std_zone_name() == std::wstring(L"MST"));
  check("Names", az2.std_zone_name() == std::wstring(L"MST"));
  check("Names", az1.dst_zone_name() == std::wstring(L""));
  check("Names", az2.dst_zone_name() == std::wstring(L""));
  check("to posix string", 
        az1.to_posix_string() == std::wstring(L"MST-07"));
  check("to posix string", 
        az2.to_posix_string() == std::wstring(L"MST-07"));
 

  // bizzar time zone spec to fully test parsing
  std::cout << "\nFictitious time zone" << std::endl;
  w_posix_time_zone bz(L"BST+11:21:15BDT-00:28,M2.2.4/03:15:42,M11.5.2/01:08:53");
  check("hast dst", bz.has_dst());
  check("UTC offset", bz.base_utc_offset() == time_duration(11,21,15));
  check("Abbrev", bz.std_zone_abbrev() == std::wstring(L"BST"));
  check("Abbrev", bz.dst_zone_abbrev() == std::wstring(L"BDT"));
  check("dst offset", bz.dst_offset() == time_duration(0,-28,0));
  check("dst start", bz.dst_local_start_time(1962) ==
      ptime(date(1962,Feb,8),time_duration(3,15,42)));
  check("dst end", bz.dst_local_end_time(1962) ==
      ptime(date(1962,Nov,27),time_duration(1,8,53)));
  
  // only checking start & end rules w/ 'J' notation
  std::cout << "\n'J' notation Start/End rule tests..." << std::endl;
  w_posix_time_zone la1(specs[4]); // "PST-08PDT,J124,J310"
  //w_posix_time_zone la1("PST-08PDT,J1,J365");// Jan1/Dec31 
  check("dst start", la1.dst_local_start_time(2003) == 
      ptime(date(2003,Feb,15),time_duration(1,30,0)));
  check("dst end", la1.dst_local_end_time(2003) == 
      ptime(date(2003,Nov,6),time_duration(2,0,0)));
  /* NOTE: la1 was created from a 'J' notation string but to_posix_string
   * returns an 'n' notation string. The difference between the two 
   * is Feb-29 is always counted in an 'n' notation string and numbering 
   * starts at zero ('J' notation starts at one). 
   * Every possible date spec that can be written in 'J' notation can also
   * be written in 'n' notation. The reverse is not true so 'n' notation 
   * is used as the output for to_posix_string(). */
  check("to posix string", 
        la1.to_posix_string() == std::wstring(L"PST-08PDT+01,45/01:30,310/02:00"));
  
  // only checking start & end rules w/ 'n' notation
  std::cout << "\n'n' notation Start/End rule tests..." << std::endl;
  w_posix_time_zone la2(specs[5]); // "PST-08PDT,124,310"
  //w_posix_time_zone la2("PST-08PDT,0,365");// Jan1/Dec31 
  check("dst start", la2.dst_local_start_time(2003) == 
      ptime(date(2003,Feb,15),time_duration(2,0,0)));
  check("dst end", la2.dst_local_end_time(2003) == 
      ptime(date(2003,Nov,6),time_duration(0,30,0)));
  check("to posix string", 
        la2.to_posix_string() == std::wstring(L"PST-08PDT+01,45/02:00,310/00:30"));

  // bad posix time zone strings tests
  std::cout << "\nInvalid time zone string tests..." << std::endl;
  try {
    w_posix_time_zone badz(L"EST-13");
    check("Exception not thrown: bad UTC offset", false);
  }catch(bad_offset& boff){
    std::string msg(boff.what());
    check("Exception caught: "+msg , true);
  }
  try {
    w_posix_time_zone badz(L"EST-5EDT24:00:01,J124/1:30,J310");
    check("Exception not thrown: bad DST adjust", false);
  }catch(bad_adjustment& badj){
    std::string msg(badj.what());
    check("Exception caught: "+msg , true);
  }
  try {
    w_posix_time_zone badz(L"EST-5EDT01:00:00,J124/-1:30,J310");
    check("Exception not thrown: bad DST start/end offset", false);
  }catch(bad_offset& boff){
    std::string msg(boff.what());
    check("Exception caught: "+msg , true);
  }
  try {
    w_posix_time_zone badz(L"EST-5EDT01:00:00,J124/1:30,J370");
    check("Exception not thrown: invalid date spec", false);
  }catch(boost::gregorian::bad_day_of_month& boff){
    std::string msg(boff.what());
    check("Exception caught: "+msg , true);
  }catch(boost::gregorian::bad_month& boff){
    std::string msg(boff.what());
    check("Exception caught: "+msg , true);
  }catch(...){
    check("Unexpected exception caught: ", false);
  }

  std::cout << "\nTest some Central Europe specs" << std::endl;

  //Test a timezone spec on the positive side of the UTC line.
  //This is the time for central europe which is one hour in front of UTC
  //Note these Summer time transition rules aren't actually correct.
  w_posix_time_zone cet_tz(L"CET+01:00:00EDT+01:00:00,M4.1.0/02:00:00,M10.5.0/02:00:00");
  check("Has DST", cet_tz.has_dst());
  check("UTC offset", cet_tz.base_utc_offset() == hours(1));
  check("Abbrevs", cet_tz.std_zone_abbrev() == std::wstring(L"CET"));
//   check("Abbrevs", nyc2.std_zone_abbrev() == std::wstring("EST"));

  std::cout << "\nTest some Central Austrialia UTC+8:30" << std::endl;

  //Test a timezone spec on the positive side of the UTC line.
  //This is the time for central europe which is one hour in front of UTC
  //Note these Summer time transition rules aren't actually correct.
  w_posix_time_zone caus_tz(L"CAS+08:30:00CDT+01:00:00,M4.1.0/02:00:00,M10.5.0/02:00:00");
  check("Has DST", caus_tz.has_dst());
  check("UTC offset", caus_tz.base_utc_offset() == hours(8)+minutes(30));
  check("Abbrevs", caus_tz.std_zone_abbrev() == std::wstring(L"CAS"));
//   check("Abbrevs", nyc2.std_zone_abbrev() == std::wstring("EST"));

  {
    /**** first/last of month Julian & non-Julian tests ****/
    // Mar-01 & Oct-31, count begins at 1
    std::wstring spec(L"FST+3FDT,J60,J304"); 
    w_posix_time_zone fl_1(spec);
    check("Julian First/last of month", fl_1.dst_local_start_time(2003) == 
        ptime(date(2003,Mar,1),hours(2)));
    check("Julian First/last of month", fl_1.dst_local_end_time(2003) == 
        ptime(date(2003,Oct,31),hours(2)));
    check("Julian First/last of month", fl_1.dst_local_start_time(2004) == 
        ptime(date(2004,Mar,1),hours(2)));
    check("Julian First/last of month", fl_1.dst_local_end_time(2004) == 
        ptime(date(2004,Oct,31),hours(2)));
    
    // Mar-01 & Oct-31 Non-leap year, count begins at 0
    spec = L"FST+3FDT,59,304"; // "304" is not a mistake here, see posix_time_zone docs
    w_posix_time_zone fl_2(spec);
    try{
      check("Non-Julian First/last of month", fl_2.dst_local_start_time(2003) ==
          ptime(date(2003,Mar,1),hours(2)));
    }catch(std::exception& e){
      check("Expected exception caught for Non-Julian day of 59, in non-leap year (Feb-29)", true);
    }
    check("Non-Julian First/last of month", fl_2.dst_local_end_time(2003) == 
        ptime(date(2003,Oct,31),hours(2)));
    
    // Mar-01 & Oct-31 leap year, count begins at 0
    spec = L"FST+3FDT,60,304"; 
    w_posix_time_zone fl_3(spec);
    check("Non-Julian First/last of month", fl_3.dst_local_start_time(2004) ==
        ptime(date(2004,Mar,1),hours(2)));
    check("Non-Julian First/last of month", fl_3.dst_local_end_time(2004) ==
        ptime(date(2004,Oct,31),hours(2)));
  }

  printTestStats();
  return 0;
}

