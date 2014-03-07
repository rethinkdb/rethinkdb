/* Copyright (c) 2003-2005 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */


#include "../testfrmwk.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time/custom_time_zone.hpp"
#include "boost/date_time/local_time/local_time_types.hpp"
#include "boost/date_time/local_time/tz_database.hpp"
#include "boost/date_time/local_time/posix_time_zone.hpp"
#include <iostream>

bool run_bad_field_count_test();

int main(){
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  using namespace boost::local_time;
  
  /* NOTE: The testlocal_time_facet tests required full names 
   * be added to some of the date_time_zonespec.csv entries. The 
   * tests here also use those full names. Those entries are:
   * Chicago, Denver, Los_Angeles, New_York, and Phoenix
   * have all had full names added */
 
  // run the exception tests first
  try{
    tz_database tz_db;
    tz_db.load_from_file("missing_file.csv"); // file does not exist
  }catch(data_not_accessible& e){
    check("Caught Missing data file exception", true);
  }catch(...){
    check("Caught first unexpected exception", false);
  }
  check("Caught Bad field count exception", run_bad_field_count_test());

  /* This test-file is usually run from either $BOOST_ROOT/status, or 
   * $BOOST_ROOT/libs/date_time/test. Therefore, the relative path 
   * to the data file this test depends on will be one of two 
   * possible paths.
   *
   * If the first attempt at opening the data file fails, an exception 
   * will be thrown. The handling of that exception consists of 
   * attempting to open it again but from a different location. If that 
   * also fails, we abort the test. */
  tz_database tz_db;
  try {
    // first try to find the data file from the test dir
    tz_db.load_from_file("../data/date_time_zonespec.csv");
  }catch(data_not_accessible& e) {
    // couldn't find the data file so assume we are being run from 
    // boost_root/status and try again
    tz_db.load_from_file("../libs/date_time/data/date_time_zonespec.csv");
  }catch(...) {
    check("Cannot locate data file - aborting.", false);
    return printTestStats();
  }
    
  time_zone_ptr bad_tz = tz_db.time_zone_from_region("Invalid/name");
  check("Expected null pointer return", bad_tz == time_zone_ptr()); 

  time_zone_ptr nyc_test = tz_db.time_zone_from_region("America/New_York");
  check("nyc Valid pointer", nyc_test != time_zone_ptr() ); 
  check("nyc Abbreviations",nyc_test->std_zone_abbrev() == std::string("EST")); 
  check("nyc Full Name", nyc_test->std_zone_name() == std::string("Eastern Standard Time")); 
  check("nyc Abbreviations",nyc_test->dst_zone_abbrev() == std::string("EDT")); 
  //std::cout << nyc_test->std_zone_name() << std::endl;
  check("nyc Full Name", nyc_test->dst_zone_name() == std::string("Eastern Daylight Time")); 
  check("nyc GMT Offset", nyc_test->base_utc_offset() == hours(-5));
  check("nyc DST Offset", nyc_test->dst_offset() == hours(1));
  //std::cout << nyc_test->dst_local_start_time(2004) << std::endl;
  check("nyc dst start",  nyc_test->dst_local_start_time(2007) == ptime(date(2007, Mar, 11), hours(2)));
  check("nyc dst end", nyc_test->dst_local_end_time(2007) == ptime(date(2007, Nov, 4), hours(2)));
  check("nyc has dst", nyc_test->has_dst());
  
  time_zone_ptr phx_test = tz_db.time_zone_from_region("America/Phoenix");
  check("az Valid pointer", phx_test != time_zone_ptr() ); 
  check("az Abbreviations",phx_test->std_zone_abbrev() == std::string("MST"));  
  check("az Full Name", phx_test->std_zone_name() == std::string("Mountain Standard Time"));  
  check("az Abbreviations", phx_test->dst_zone_abbrev() == std::string(""));  
  check("az Full Name", phx_test->dst_zone_name() == std::string(""));  
  check("az GMT Offset", phx_test->base_utc_offset() == hours(-7));
  check("az DST Offset", phx_test->dst_offset() == hours(0));
  //std::cout << phx_test->dst_local_start_time(2004) << std::endl;
  check("az has dst", phx_test->has_dst() == false);

  //Now add and retrieve a Posix tz spec from the database
  time_zone_ptr eastern(new posix_time_zone("EST-05:00:00EDT+01:00:00,M4.1.0/02:00:00,M10.5.0/02:00:00"));
  tz_db.add_record("United States/Eastern", eastern);
  time_zone_ptr eastern_test = tz_db.time_zone_from_region("United States/Eastern");
  check("eastern Valid pointer", eastern_test != time_zone_ptr() );
  check("eastern Abbreviations", 
      eastern_test->std_zone_abbrev() == std::string("EST"));  
  check("eastern Abbreviations", 
      eastern_test->std_zone_name() == std::string("EST"));  
  check("eastern Abbreviations", 
      eastern_test->dst_zone_abbrev() == std::string("EDT"));  
  check("eastern Abbreviations", 
      eastern_test->dst_zone_name() == std::string("EDT"));  
  check("eastern GMT Offset", eastern_test->base_utc_offset() == hours(-5));
  check("eastern dst start",  eastern_test->dst_local_start_time(2004) == ptime(date(2004, Apr, 4), hours(2)));
  check("eastern dst end", eastern_test->dst_local_end_time(2004) == ptime(date(2004, Oct, 31), hours(2)));
  check("eastern  has dst", eastern_test->has_dst() == true);


  return printTestStats();
}

/* This test only checks to make sure the bad_field_count exception
 * is properly thrown. It does not pay any attention to any other 
 * exception, those are tested elsewhere. */
bool run_bad_field_count_test()
{
  using namespace boost::local_time;
  bool caught_bfc = false;
  tz_database other_db;
  try{
    other_db.load_from_file("local_time/poorly_formed_zonespec.csv");
  }catch(bad_field_count& be){
    caught_bfc = true;
  }catch(...) {
    // do nothing (file not found)
  }
  try{
    other_db.load_from_file("../libs/date_time/test/local_time/poorly_formed_zonespec.csv");
  }catch(bad_field_count& be){
    caught_bfc = true;
  }catch(...) {
    // do nothing (file not found)
  }
  return caught_bfc;
}

