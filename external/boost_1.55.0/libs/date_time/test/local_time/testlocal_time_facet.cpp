
/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */

#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include "boost/date_time/local_time/local_time.hpp"
#include "../testfrmwk.hpp"


template<class temporal_type, typename charT>
inline
void
teststreaming(std::string const& testname,
              temporal_type value,
              std::basic_string<charT> const& expected_result,
              const std::locale& locale)
{
  std::basic_stringstream<charT> ss;
  ss.imbue(locale);
  ss << value;

  if (!check(testname, ss.str() == expected_result))
#if !defined(BOOST_NO_STD_WSTRING)
    std::wcout << L"Expected: \"" << expected_result.c_str() << L"\"\nGot:      \"" << ss.str().c_str() << L"\"" << std::endl;
#else
    std::cout << "Expected: \"" << expected_result.c_str() << "\"\nGot:      \"" << ss.str().c_str() << L"\"" << std::endl;
#endif
}

int main(){
  /* use the tz_database for the time zones. 
   * Chicago, Denver, Los_Angeles, New_Tork, and Phoenix 
   * have all had full names added */
  using namespace boost;
  using namespace boost::local_time;
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  tz_database time_zones;

  try {
    // first try to find the data file from the test dir
    time_zones.load_from_file("../data/date_time_zonespec.csv");
  }
  catch(const data_not_accessible& e) {
    // couldn't find the data file so assume we are being run from 
    // boost_root/status and try again
    try {
      time_zones.load_from_file("../libs/date_time/data/date_time_zonespec.csv");
    }
    catch(const data_not_accessible&) {
      check("Cannot locate data file - aborting.", false);
      return printTestStats();
    }
  }

  time_zone_ptr utc;
  time_zone_ptr chicago = time_zones.time_zone_from_region("America/Chicago");
  time_zone_ptr denver = time_zones.time_zone_from_region("America/Denver");
  time_zone_ptr la = time_zones.time_zone_from_region("America/Los_Angeles");
  time_zone_ptr nyc = time_zones.time_zone_from_region("America/New_York");
  time_zone_ptr phx = time_zones.time_zone_from_region("America/Phoenix");
  // sydney does not have full time zone names in the tz_database
  time_zone_ptr sydney = time_zones.time_zone_from_region("Australia/Sydney");

  ptime a_time(date(2004,Dec,15), hours(12));
  ptime b_time(date(2004,Aug,15), hours(12));
  local_date_time ldt1(a_time, utc);
  local_date_time ldt2(b_time, chicago);
  local_date_time ldt3(a_time, denver);
  local_date_time ldt4(b_time, la);
  local_date_time ldt5(a_time, nyc);
  local_date_time ldt6(b_time, phx);

  local_time_period ltp1(ldt1, hours(10) + minutes(24) + seconds(5));
  local_time_period ltp2(ldt4, hours(15) + minutes(20) + seconds(41));

  typedef boost::date_time::time_facet<local_date_time, char> ldt_facet;
  //ldt_facet* timefacet = new ldt_facet("%c %Z"); // full name
  ldt_facet* timefacet = new ldt_facet("%a %b %d %H:%M:%S %Y %Z"); // full name
  std::locale loc1(std::locale::classic(), timefacet);
  
  typedef boost::date_time::time_facet<ptime, char> pt_facet;
  //pt_facet* ptimefacet1 = new pt_facet("%c %Z"); // show that zone is ignored
  pt_facet* ptimefacet1 = new pt_facet("%a %b %d %H:%M:%S %Y %Z"); // show that zone is ignored
  std::locale loc2(std::locale::classic(), ptimefacet1);


  std::cout << "\nFull time zone names tests" << std::endl;
  teststreaming("ptime with %Z flag\n", a_time, std::string("Wed Dec 15 12:00:00 2004") , loc2);
  
  teststreaming("UTC local_date_time", ldt1, std::string("Wed Dec 15 12:00:00 2004 Coordinated Universal Time"), loc1);
  teststreaming("Chicago in summer", ldt2, std::string("Sun Aug 15 07:00:00 2004 Central Daylight Time") , loc1);
  teststreaming("Denver in winter", ldt3, std::string("Wed Dec 15 05:00:00 2004 Mountain Standard Time"), loc1);
  teststreaming("Los Angeles in summer", ldt4, std::string("Sun Aug 15 05:00:00 2004 Pacific Daylight Time"), loc1);
  teststreaming("New York in winter", ldt5, std::string("Wed Dec 15 07:00:00 2004 Eastern Standard Time"), loc1);
  teststreaming("Phoenix in Summer", ldt6, std::string("Sun Aug 15 05:00:00 2004 Mountain Standard Time"), loc1);

  teststreaming("UTC local_time_period", ltp1, std::string("[Wed Dec 15 12:00:00 2004 Coordinated Universal Time/Wed Dec 15 22:24:04 2004 Coordinated Universal Time]"), loc1);
  teststreaming("LA local_time_period", ltp2, std::string("[Sun Aug 15 05:00:00 2004 Pacific Daylight Time/Sun Aug 15 20:20:40 2004 Pacific Daylight Time]"), loc1);

  //ptimefacet1->format("%c %z"); // show that zone abbrev is ignored
  ptimefacet1->format("%a %b %d %H:%M:%S %Y %z"); // show that zone abbrev is ignored
  std::cout << "\nTime zone abbreviation tests" << std::endl;
  teststreaming("ptime with %z flag\n", a_time, std::string("Wed Dec 15 12:00:00 2004") , loc2);

  // using standard format
  //timefacet->format("%c %z"); // abbreviated zone
  timefacet->format("%a %b %d %H:%M:%S %Y %z"); // abbreviated zone
  teststreaming("UTC local_date_time", ldt1, std::string("Wed Dec 15 12:00:00 2004 UTC"), loc1);
  teststreaming("Chicago in summer", ldt2, std::string("Sun Aug 15 07:00:00 2004 CDT") , loc1);
  teststreaming("Denver in winter", ldt3, std::string("Wed Dec 15 05:00:00 2004 MST"), loc1);
  teststreaming("Los Angeles in summer", ldt4, std::string("Sun Aug 15 05:00:00 2004 PDT"), loc1);
  teststreaming("New York in winter", ldt5, std::string("Wed Dec 15 07:00:00 2004 EST"), loc1);
  teststreaming("Phoenix in Summer", ldt6, std::string("Sun Aug 15 05:00:00 2004 MST"), loc1);

  // iso format
  // show that zone offset is ignored
  ptimefacet1->format(pt_facet::iso_time_format_specifier); 
  std::cout << "\nLocal time iso format tests" << std::endl;
  teststreaming("ptime with iso format\n", a_time, 
                std::string("20041215T120000") , loc2);

  timefacet->format(ldt_facet::iso_time_format_specifier);
  teststreaming("UTC local_date_time", ldt1, 
                std::string("20041215T120000Z"), loc1);
  teststreaming("Chicago in summer", ldt2, 
                std::string("20040815T070000-0500") , loc1);
  teststreaming("Denver in winter", ldt3, 
                std::string("20041215T050000-0700"), loc1);
  teststreaming("Los Angeles in summer", ldt4, 
                std::string("20040815T050000-0700"), loc1);
  teststreaming("New York in winter", ldt5, 
                std::string("20041215T070000-0500"), loc1);
  teststreaming("Phoenix in Summer", ldt6, 
                std::string("20040815T050000-0700"), loc1);
  teststreaming("Sydney in December", ldt1.local_time_in(sydney), 
                std::string("20041215T230000+1100"), loc1);
  
  // iso extended format
  // show that zone offset is ignored
  ptimefacet1->format(pt_facet::iso_time_format_extended_specifier); 
  std::cout << "\nLocal time iso_extended tests" << std::endl;
  teststreaming("ptime with iso extended format\n", a_time, 
                std::string("2004-12-15 12:00:00") , loc2);

  timefacet->format(ldt_facet::iso_time_format_extended_specifier);
  teststreaming("UTC local_date_time", ldt1, 
                std::string("2004-12-15 12:00:00Z"), loc1);
  teststreaming("Chicago in summer", ldt2, 
                std::string("2004-08-15 07:00:00-05:00") , loc1);
  teststreaming("Denver in winter", ldt3, 
                std::string("2004-12-15 05:00:00-07:00"), loc1);
  teststreaming("Los Angeles in summer", ldt4, 
                std::string("2004-08-15 05:00:00-07:00"), loc1);
  teststreaming("New York in winter", ldt5, 
                std::string("2004-12-15 07:00:00-05:00"), loc1);
  teststreaming("Phoenix in Summer", ldt6, 
                std::string("2004-08-15 05:00:00-07:00"), loc1);
  teststreaming("Sydney in December", ldt1.local_time_in(sydney), 
                std::string("2004-12-15 23:00:00+11:00"), loc1);

#if !defined(BOOST_NO_STD_WSTRING)

  typedef boost::date_time::time_facet<local_date_time, wchar_t> wldt_facet;
  //wldt_facet* wtimefacet = new wldt_facet(L"%c %Z"); // full name
  wldt_facet* wtimefacet = new wldt_facet(L"%a %b %d %H:%M:%S %Y %Z"); // full name
  std::locale loc3(std::locale::classic(), wtimefacet);
  
  /* Again, wide stream tests are more thoroughly done in the 
   * time_facet tests. Here we just need to show that they work */
  std::cout << "\nFull time zone names tests - wide stream" << std::endl;
  teststreaming("UTC local_date_time", ldt1, std::wstring(L"Wed Dec 15 12:00:00 2004 Coordinated Universal Time"), loc3);
  teststreaming("Chicago in summer", ldt2, std::wstring(L"Sun Aug 15 07:00:00 2004 Central Daylight Time") , loc3);
 
  teststreaming("UTC local_time_period", ltp1, std::wstring(L"[Wed Dec 15 12:00:00 2004 Coordinated Universal Time/Wed Dec 15 22:24:04 2004 Coordinated Universal Time]"), loc3);
  teststreaming("LA local_time_period", ltp2, std::wstring(L"[Sun Aug 15 05:00:00 2004 Pacific Daylight Time/Sun Aug 15 20:20:40 2004 Pacific Daylight Time]"), loc3);

  //wtimefacet->format(L"%c %z"); // abbrev
  wtimefacet->format(L"%a %b %d %H:%M:%S %Y %z"); // abbrev
  std::cout << "\nAbbreviated time zone names tests - wide stream" << std::endl;
  teststreaming("UTC local_date_time", ldt1, std::wstring(L"Wed Dec 15 12:00:00 2004 UTC"), loc3);
  teststreaming("Phoenix in Summer", ldt6, std::wstring(L"Sun Aug 15 05:00:00 2004 MST"), loc3);

#endif // BOOST_NO_STD_WSTRING
  
  return printTestStats();
}
