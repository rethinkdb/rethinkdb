/* Copyright (c) 2003-2005 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2012-09-22 09:21:43 -0700 (Sat, 22 Sep 2012) $
 */


#include "boost/date_time/local_time/custom_time_zone.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"
// #include "boost/date_time/local_time/posix_time_zone.hpp"
#include "../testfrmwk.hpp"
//#include "boost/date_time/c_time.hpp"
#include <iostream>

#include <sstream>
// function eases testing
std::string tm_out(const tm& ptr){
  std::stringstream ss;

  ss 
    << ptr.tm_wday << ' ' << ptr.tm_yday << ' '
    << std::setw(2) << std::setfill('0') << ptr.tm_mon + 1 << '/'
    << std::setw(2) << std::setfill('0') << ptr.tm_mday << '/'
    << std::setw(2) << std::setfill('0') << ptr.tm_year + 1900 << ' '
    << std::setw(2) << std::setfill('0') << ptr.tm_hour << ':'
    << std::setw(2) << std::setfill('0') << ptr.tm_min << ':'
    << std::setw(2) << std::setfill('0') << ptr.tm_sec << ' ';
  if(ptr.tm_isdst >= 0){
    ss << (ptr.tm_isdst ? "DST" : "STD");
  }
  else{
    ss << "DST/STD unknown";
  }
  return ss.str();
}

int
main()
{

  using namespace boost::gregorian;
  using namespace boost::posix_time;
  using namespace boost::local_time;

  // since local_date_time inherits it's math operations from time, the 
  // tests here only show that the operations work. The thorough testing
  // of these operations is done in the posix_time tests

  try {
    time_zone_ptr az_tz(new posix_time_zone("MST-07"));
    time_zone_ptr ny_tz(new posix_time_zone("EST-05EDT,M4.1.0,M10.5.0"));
    // EST & EST for sydney is correct, according to zoneinfo files
    time_zone_ptr sydney(new posix_time_zone("EST+10EST,M10.5.0,M3.5.0/03:00"));
    time_zone_ptr null_tz;
    date d(2003, 12, 20);
    hours h(12);
    ptime t(d,h);
    local_date_time az_time(t, az_tz); // ptime constructor is a UTC time
    
    check("Zone abbreviation", az_time.zone()->std_zone_abbrev() == std::string("MST"));
    check("base offset", az_time.zone()->base_utc_offset() == hours(-7));
    check("zone has dst", az_time.zone()->has_dst() == false);
    check("is_dst check", az_time.is_dst() == false);
    check("to_string: " + az_time.to_string(), 
          az_time.to_string() == "2003-Dec-20 05:00:00 MST");
   

    
    std::cout << "\nChecking copy construction" << std::endl;
    local_date_time az_time2(az_time); //copy constructor
    // Now test the copy
    check("is_dst check", az_time2.is_dst() == false);
    check("to_string: " + az_time2.to_string(), 
          az_time2.to_string() == "2003-Dec-20 05:00:00 MST");
    check("zone has dst", az_time2.zone()->has_dst() == false);
    check("base offset", az_time2.zone()->base_utc_offset() == hours(-7));
    

    std::cout << "\nChecking special_value construction" << std::endl;
    // since local_date_time inherits its special value operatorations 
    // from time, we only need to show here that they work as thorough 
    // testing is done in the posix_time tests
    ptime svpt(not_a_date_time);
    local_date_time sv_time(svpt, ny_tz);
    check("is special_value", sv_time.is_not_a_date_time());
    check("to_string: " + sv_time.to_string(), 
          sv_time.to_string() == "not-a-date-time");
    check("is_dst", sv_time.is_dst() == false);
    local_date_time sv_time2(pos_infin);
    check("is special_value", sv_time2.is_pos_infinity());
    check("to_string: " + sv_time2.to_string(), 
          sv_time2.to_string() == "+infinity");
    check("is_dst", sv_time2.is_dst() == false);
    sv_time2 += days(12); // add a duration to a special value
    check("Add a duration to a special value", sv_time2.is_pos_infinity());

    local_date_time sv_time3(max_date_time, ny_tz);
#ifdef BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
    check("max_date_time to_string: " + sv_time3.to_string(), 
          sv_time3.to_string() == "9999-Dec-31 18:59:59.999999999 EST");
#else
    check("max_date_time to_string: " + sv_time3.to_string(), 
          sv_time3.to_string() == "9999-Dec-31 18:59:59.999999 EST");
#endif

    try {
      local_date_time sv_time4(min_date_time);
      check("min_date_time to_string: " + sv_time4.to_string(), 
            sv_time4.to_string() == "1400-Jan-01 00:00:00 UTC");
    }
    catch (std::exception& e) {
      check("min_date_time to_string -- exception" , false);
      std::cout << "Exception is : " << e.what() << std::endl;
    }

/** todo  -- this will cause an out of range when min_date is adjusted for ny_tz
    local_date_time sv_time5(min_date_time, ny_tz);
    std::cout << sv_time5.to_string() << std::endl;
**/

    std::cout << "\nChecking calc_options construction" << std::endl;
    { // invalid NADT
      date d(2004, Apr, 4);
      time_duration td(2,30,0); // invalid local time in ny_tz
      local_date_time calcop(d, td, ny_tz, local_date_time::NOT_DATE_TIME_ON_ERROR);
      check("is NADT", calcop.is_not_a_date_time());
    }
    { // invalid exception
      date d(2004, Apr, 4);
      time_duration td(2,30,0); // invalid local time in ny_tz
      try{
        local_date_time calcop(d, td, ny_tz, local_date_time::EXCEPTION_ON_ERROR);
        check("Did not catch expected exception", false);
      }catch(time_label_invalid& /*i*/){
        check("Caught expected exception", true);
      }catch(...){
        check("Caught unexpected exception", false);
      }
    }
    { // ambig NADT
      date d(2004, Oct, 31);
      time_duration td(1,30,0); // ambig local time in ny_tz
      local_date_time calcop(d, td, ny_tz, local_date_time::NOT_DATE_TIME_ON_ERROR);
      check("is NADT", calcop.is_not_a_date_time());
    }
    { // ambig exception
      date d(2004, Oct, 31);
      time_duration td(1,30,0); // ambig local time in ny_tz
      try{
        local_date_time calcop(d, td, ny_tz, local_date_time::EXCEPTION_ON_ERROR);
        check("Did not catch expected exception", false);
      }catch(ambiguous_result& /*a*/){
        check("Caught expected exception", true);
      }catch(...){
        check("Caught unexpected exception", false);
      }
    }

    
    //Now construct with a date and time
    std::cout << "\nChecking construct with date and time_duration" << std::endl;
    local_date_time az_time3(d, h, az_tz, false);
    check("Zone abbreviation", az_time3.zone()->std_zone_abbrev() == std::string("MST"));
    check("base offset", az_time3.zone()->base_utc_offset() == hours(-7));
    check("base offset", az_time3.zone()->has_dst() == false);
    check("is_dst check", az_time3.is_dst() == false);
    check("to_string: " + az_time3.to_string(), 
          az_time3.to_string() == "2003-Dec-20 12:00:00 MST");
    
    // construct with a null tz
    //local_date_time null_tz_time(d, h, null_tz, false);
    local_date_time null_tz_time(d, h, null_tz, true);
    // TODO: how to handle calls to null_tz_time.zone()->...
    check("is_dst check", null_tz_time.is_dst() == false);
    check("to_string: " + null_tz_time.to_string(), 
          null_tz_time.to_string() == "2003-Dec-20 12:00:00 UTC");
    
    //Now construct with a date and time - invalid parameters
    try{
      local_date_time blt(d, h, ny_tz, true);
      check("Did not catch expected exception (dst_not_valid)", false);
    }catch(dst_not_valid& d){
      check(std::string("Caught expected exception (dst_not_valid) ") + d.what(), true);
    }catch(std::exception& e){
      check(std::string("Caught unexpected exception ") + e.what(), false);
    }
    try{
      local_date_time blt(date(2004,Apr,4), time_duration(2,30,0), ny_tz, true);
      check("Did not catch expected exception (Invalid_Time_Label)", false);
    }catch(time_label_invalid& e){
      check(std::string("Caught expected exception (Invalid_Time_Label) ") + e.what(), true);
    }catch(std::exception& e){
      check(std::string("Caught unexpected exception ") + e.what(), false);
    }
    
    
    // thorough is_dst() tests, tests againts null_tz and non dst tz are
    // done where those local times were tested
    {
      date d(2004,Apr,4);
      time_duration td(1,15,0); // local
      local_date_time lt1(d,td,ny_tz,false);
      local_date_time lt2(ptime(d,time_duration(6,15,0)), ny_tz);
      check("are local_times equal", lt1.utc_time() == lt2.utc_time());
      check("is_dst - transition in 1", lt1.is_dst() == false);
      check("is_dst - transition in 2", lt2.is_dst() == false);
      lt1 += hours(1);
      lt2 += hours(1);
      check("is_dst - transition in 1", lt1.is_dst() == true);
      check("is_dst - transition in 2", lt2.is_dst() == true);
    }
    {
      date d(2004,Oct,31);
      time_duration td(1,15,0); // local
      local_date_time lt1(d,td,ny_tz,true);
      /*try{
        //local_date_time lt1(d,td,ny_tz,false);
        local_date_time lt1(d,td,ny_tz,true);
        std::cout << "no exception thrown" << std::endl;
      }catch(time_label_invalid& e){
        std::cout << "caught: " << e.what() << std::endl;
      }*/
      local_date_time lt2(ptime(d,time_duration(5,15,0)), ny_tz);
      check("are local_times equal", lt1.utc_time() == lt2.utc_time());
      check("is_dst - transition out 1", lt1.is_dst() == true);
      check("is_dst - transition out 2", lt2.is_dst() == true);
      lt1 += hours(1);
      lt2 += hours(1);
      check("is_dst - transition out 1", lt1.is_dst() == false);
      check("is_dst - transition out 2", lt2.is_dst() == false);
    }
    { // southern hemisphere 
      date d(2004,Oct,31);
      time_duration td(1,15,0); // local
      local_date_time lt1(d,td,sydney,false);
      check("is_dst - transition in (sydney)", lt1.is_dst() == false);
      lt1 += hours(1);
      check("is_dst - transition in (sydney)", lt1.is_dst() == true);
    }
    {
      date d(2004,Mar,28);
      time_duration td(2,15,0); // local; sydney has a weird trans time
      local_date_time lt1(d,td,sydney,true);
      check("is_dst - transition out (sydney)", lt1.is_dst() == true);
      lt1 += hours(1);
      check("is_dst - transition out (sydney)", lt1.is_dst() == false);
    }


    
    std::cout << "\nTest conversion of time zone from Arizona to New York" << std::endl;
    local_date_time ny_time = az_time.local_time_in(ny_tz);
    check("Zone abbreviation", ny_time.zone()->std_zone_abbrev() == std::string("EST"));
    check("base offset", ny_time.zone()->base_utc_offset() == hours(-5));
    check("base offset", ny_time.zone()->has_dst() == true);
    check("to_string: " + ny_time.to_string(), 
          ny_time.to_string() == "2003-Dec-20 07:00:00 EST");
    ny_time += hours(3);
    check("to_string after add 3 hours: " + ny_time.to_string(), 
          ny_time.to_string() == "2003-Dec-20 10:00:00 EST");
    ny_time += days(3);
    check("to_string after add 3 days: " + ny_time.to_string(), 
          ny_time.to_string() == "2003-Dec-23 10:00:00 EST");
    
  
    { // test comparisons & math operations
      date d(2003, Aug, 28);
      ptime sv_pt(pos_infin);
      local_date_time sv_lt(sv_pt, ny_tz);
      ptime utc_pt(d, hours(12));
      // all 4 of the following local times happen at the same instant
      // so they are all equal
      local_date_time utc_lt(utc_pt, null_tz);           // noon in utc
      local_date_time az_lt(d, hours(5), az_tz, false);  // 5am local std
      local_date_time ny_lt(d, hours(8), ny_tz, true);   // 8am local dst
      local_date_time au_lt(d, hours(22), sydney, false);// 10pm local std

      check("local_date_time to tm",
          std::string("4 239 08/28/2003 05:00:00 STD") == tm_out(to_tm(az_lt)));
      check("local_date_time to tm",
          std::string("4 239 08/28/2003 08:00:00 DST") == tm_out(to_tm(ny_lt)));
      check("local_date_time to tm",
          std::string("4 239 08/28/2003 22:00:00 STD") == tm_out(to_tm(au_lt)));

      try{
        local_date_time ldt(not_a_date_time);
        tm ldt_tm = to_tm(ldt);
        check("Exception not thrown (special_value to_tm)", false);
	//does nothing useful but stops compiler from complaining about unused ldt_tm
	std::cout << ldt_tm.tm_sec << std::endl; 
      }catch(std::out_of_range& e){
        check("Caught expected exception (special_value to_tm)", true);
      }catch(...){
        check("Caught un-expected exception (special_value to_tm)", false);
      }
      // check that all are equal to sv_pt
      check("local == utc", az_lt == utc_lt); 
      check("local == utc", ny_lt == utc_lt); 
      check("local == utc", au_lt == utc_lt); 
      check("local <= utc", au_lt <= utc_lt); 
      check("local >= utc", au_lt >= utc_lt); 
      check("local == local", az_lt == ny_lt); 
      check("local < local", az_lt < ny_lt+seconds(1)); 
      check("local > local", az_lt+seconds(1) >  ny_lt); 
      check("local <= local", az_lt <= ny_lt); 
      check("local >= local", az_lt >=  ny_lt); 
      check("local != local", az_lt+seconds(1) !=  ny_lt); 
      
      au_lt += hours(1);
      check("local != after +=", au_lt != utc_lt); 
      check("local <= after +=", utc_lt <= au_lt);  
      check("local >= after +=", au_lt >= utc_lt); 
      check("local < after +=", utc_lt < au_lt); 
      check("local > after +=", au_lt > utc_lt);
      au_lt -= hours(1);
      check("local == utc after -=", au_lt == utc_lt); 

      check("local + days", 
          (az_lt + days(2)).to_string() == "2003-Aug-30 05:00:00 MST");
      check("local - days", 
          (az_lt - days(2)).to_string() == "2003-Aug-26 05:00:00 MST");
      check("local += days", 
          (az_lt += days(2)).to_string() == "2003-Aug-30 05:00:00 MST");
      check("local -= days", 
          (az_lt -= days(2)).to_string() == "2003-Aug-28 05:00:00 MST");
      check("local + time_duration", 
          (az_lt + hours(2)).to_string() == "2003-Aug-28 07:00:00 MST");
      check("local - time_duration", 
          (az_lt - hours(2)).to_string() == "2003-Aug-28 03:00:00 MST");
      // special_values is more thoroughly tested in posix_time
      check("pos_infinity > local", sv_lt > au_lt); 
      local_date_time sv_lt2(sv_lt + days(2));
      check("pos_infin + duration == pos_infin", sv_lt2 == sv_lt);
   
#if defined(BOOST_DATE_TIME_OPTIONAL_GREGORIAN_TYPES)
      months m(2);
      years y(2);
      check("Local + months", 
          (az_lt + m).to_string() == "2003-Oct-28 05:00:00 MST");
      az_lt += m;
      check("Local += months", 
          az_lt.to_string() == "2003-Oct-28 05:00:00 MST");
      check("Local - months", 
          (az_lt - m).to_string() == "2003-Aug-28 05:00:00 MST");
      az_lt -= m;
      check("Local -= months", 
          az_lt.to_string() == "2003-Aug-28 05:00:00 MST");
      check("Local + years", 
          (az_lt + y).to_string() == "2005-Aug-28 05:00:00 MST");
      az_lt += y;
      check("Local += years", 
          az_lt.to_string() == "2005-Aug-28 05:00:00 MST");
      check("Local - years", 
          (az_lt - y).to_string() == "2003-Aug-28 05:00:00 MST");
      az_lt -= y;
      check("Local -= years", 
          az_lt.to_string() == "2003-Aug-28 05:00:00 MST");
      
#endif // BOOST_DATE_TIME_OPTIONAL_GREGORIAN_TYPES
    }
  }
  catch(std::exception& e) {
    check(std::string("test failed due to exception: ") + e.what(), false);
  }

  return printTestStats();
}

