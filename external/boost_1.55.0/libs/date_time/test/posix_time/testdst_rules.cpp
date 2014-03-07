/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_timezone_defs.hpp"
#include "../testfrmwk.hpp"

// Define dst rule for Paraguay which is transitions forward on Oct 1 and 
// back Mar 1

struct paraguay_dst_traits { 
  typedef boost::gregorian::date           date_type;
  typedef boost::gregorian::date::day_type day_type;
  typedef boost::gregorian::date::month_type month_type;
  typedef boost::gregorian::date::year_type year_type;
  typedef boost::date_time::partial_date<boost::gregorian::date> start_rule_functor;
  typedef boost::date_time::partial_date<boost::gregorian::date> end_rule_functor;
  static day_type start_day(year_type) {return 1;}
  static month_type start_month(year_type) {return boost::date_time::Oct;}
  static day_type end_day(year_type) {return 1;}
  static month_type end_month(year_type) {return boost::date_time::Mar;}
  static int dst_start_offset_minutes() { return 120;}
  static int dst_end_offset_minutes() { return 120; }
  static int dst_shift_length_minutes() { return 60; }
  static date_type local_dst_start_day(year_type year)
  {
    start_rule_functor start(start_day(year), 
                             start_month(year));
    return start.get_date(year);      
  }
  static date_type local_dst_end_day(year_type year)
  {
    end_rule_functor end(end_day(year), 
                         end_month(year));
    return end.get_date(year);      
  }


};


// see http://www.timeanddate.com/time/aboutdst.html for some info
// also 
int
main() 
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  date d(2002,Feb,1);
  ptime t(d);

  //The following defines the US dst boundaries, except that the
  //start and end dates are hard coded.
  typedef boost::date_time::us_dst_rules<date, time_duration, 120, 60> us_dst;
  date dst_start(2002,Apr, 7);
  date dst_end(2002,Oct, 27);

  ptime t3a(dst_start, time_duration(2,0,0));   //invalid time label 
  ptime t3b(dst_start, time_duration(2,59,59)); //invalid time label
  ptime t4(dst_start, time_duration(1,59,59));   //not ds 
  ptime t5(dst_start, time_duration(3,0,0));   //always dst
  ptime t6(dst_end, time_duration(0,59,59)); //is dst
  ptime t7(dst_end, time_duration(1,0,0));   //ambiguous
  ptime t8(dst_end, time_duration(1,59,59));   //ambiguous 
  ptime t9(dst_end, time_duration(2,0,0));   //always not dst

  check("dst start", us_dst::local_dst_start_day(2002) == dst_start);
  check("dst end",   us_dst::local_dst_end_day(2002) == dst_end);
  check("dst boundary",   us_dst::is_dst_boundary_day(dst_start));
  check("dst boundary",   us_dst::is_dst_boundary_day(dst_end));
  check("check if time is dst -- not",   
        us_dst::local_is_dst(t.date(), t.time_of_day())==boost::date_time::is_not_in_dst);
  check("label on dst boundary invalid", 
        us_dst::local_is_dst(t3a.date(),t3a.time_of_day())==boost::date_time::invalid_time_label);
  check("label on dst boundary invalid",   
        us_dst::local_is_dst(t3b.date(),t3b.time_of_day())==boost::date_time::invalid_time_label);
   check("check if time is dst -- not",   
         us_dst::local_is_dst(t4.date(),t4.time_of_day())==boost::date_time::is_not_in_dst);
   check("check if time is dst -- yes",   
         us_dst::local_is_dst(t5.date(),t5.time_of_day())==boost::date_time::is_in_dst);

   check("check if time is dst -- not",   
         us_dst::local_is_dst(t6.date(),t6.time_of_day())==boost::date_time::is_in_dst);
   check("check if time is dst -- ambig", 
         us_dst::local_is_dst(t7.date(),t7.time_of_day())==boost::date_time::ambiguous);
   check("check if time is dst -- ambig", 
         us_dst::local_is_dst(t8.date(),t8.time_of_day())==boost::date_time::ambiguous);
   check("check if time is dst -- not",   
         us_dst::local_is_dst(t9.date(),t9.time_of_day())==boost::date_time::is_not_in_dst);


  //Now try a local without dst
  typedef boost::date_time::null_dst_rules<date, time_duration> no_dst_adj;

  check("check null dst rules",   
        no_dst_adj::local_is_dst(t4.date(),t4.time_of_day())==boost::date_time::is_not_in_dst);
  check("check null dst rules",   
        no_dst_adj::local_is_dst(t5.date(),t5.time_of_day())==boost::date_time::is_not_in_dst);
  check("check null dst rules",   
        no_dst_adj::utc_is_dst(t4.date(),t4.time_of_day())==boost::date_time::is_not_in_dst);
  check("check null dst rules",   
        no_dst_adj::utc_is_dst(t5.date(),t5.time_of_day())==boost::date_time::is_not_in_dst);
  

  //Try a southern hemisphere adjustment calculation
  //This is following the rules for South Australia as best I can
  //decipher them.  Basically conversion to DST is last Sunday in
  //October 02:00:00 and conversion off of dst is last sunday in
  //March 02:00:00.  
  //This stuff uses the dst calculator directly...
  date dst_start2(2002,Oct,27); //last Sunday in Oct
  date dst_end2(2002,Mar,31); //last Sunday in March

  typedef boost::date_time::dst_calculator<date,time_duration> dstcalc;
  //clearly not in dst
  boost::date_time::time_is_dst_result a1 =
    dstcalc::local_is_dst(date(2002,May,1),hours(3),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);

  check("check southern not dst",  a1==boost::date_time::is_not_in_dst);

  boost::date_time::time_is_dst_result a2 =
    dstcalc::local_is_dst(date(2002,Jan,1),hours(3),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);

  check("check southern is dst",  a2==boost::date_time::is_in_dst);

  boost::date_time::time_is_dst_result a3 =
    dstcalc::local_is_dst(date(2002,Oct,28),hours(3),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);

  check("check southern is dst",  a3==boost::date_time::is_in_dst);
  boost::date_time::time_is_dst_result a4 =
    dstcalc::local_is_dst(date(2002,Oct,27),time_duration(1,59,59),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);
  check("check southern boundary-not dst",  a4==boost::date_time::is_not_in_dst);
  boost::date_time::time_is_dst_result a5 =
    dstcalc::local_is_dst(date(2002,Oct,27),hours(3),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);
  check("check southern boundary-is dst",  a5==boost::date_time::is_in_dst);
  boost::date_time::time_is_dst_result a6 =
    dstcalc::local_is_dst(date(2002,Oct,27),hours(2),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);
  check("check southern boundary-invalid time",  a6==boost::date_time::invalid_time_label);
  boost::date_time::time_is_dst_result a7 =
    dstcalc::local_is_dst(date(2002,Mar,31),time_duration(1,59,59),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);
  check("check southern boundary-is dst",  a7==boost::date_time::is_in_dst);
  boost::date_time::time_is_dst_result a8 =
    dstcalc::local_is_dst(date(2002,Mar,31),time_duration(2,0,0),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);
  check("check southern boundary-ambiguous",  a8==boost::date_time::ambiguous);
  boost::date_time::time_is_dst_result a9 =
    dstcalc::local_is_dst(date(2002,Mar,31),time_duration(2,59,59),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);
  check("check southern boundary-ambiguous",  a9==boost::date_time::ambiguous);
  boost::date_time::time_is_dst_result a10 =
    dstcalc::local_is_dst(date(2002,Mar,31),time_duration(3,0,0),
                          dst_start2, 120,
                          dst_end2, 180,
                          60);
  check("check southern boundary-not",  a10==boost::date_time::is_not_in_dst);

  /******************** post release 1 -- new dst calc engine ********/
  
  typedef boost::date_time::us_dst_trait<date> us_dst_traits;
  typedef boost::date_time::dst_calc_engine<date, time_duration, us_dst_traits>
    us_dst_calc2;
    
  {
    //  us_dst_calc2
    check("dst start", us_dst_calc2::local_dst_start_day(2002) == dst_start);
    check("dst end",   us_dst_calc2::local_dst_end_day(2002) == dst_end);
    //  std::cout << us_dst_calc2::local_dst_end_day(2002) << std::endl;
    check("dst boundary",   us_dst_calc2::is_dst_boundary_day(dst_start));
    check("dst boundary",   us_dst_calc2::is_dst_boundary_day(dst_end));
    
    check("check if time is dst -- not",   
          us_dst_calc2::local_is_dst(t.date(), t.time_of_day())==boost::date_time::is_not_in_dst);
    check("label on dst boundary invalid", 
          us_dst_calc2::local_is_dst(t3a.date(),t3a.time_of_day())==boost::date_time::invalid_time_label);
    check("label on dst boundary invalid",   
          us_dst_calc2::local_is_dst(t3b.date(),t3b.time_of_day())==boost::date_time::invalid_time_label);
    check("check if time is dst -- not",   
          us_dst_calc2::local_is_dst(t4.date(),t4.time_of_day())==boost::date_time::is_not_in_dst);
    check("check if time is dst -- yes",   
          us_dst_calc2::local_is_dst(t5.date(),t5.time_of_day())==boost::date_time::is_in_dst);

    check("check if time is dst -- not",   
          us_dst_calc2::local_is_dst(t6.date(),t6.time_of_day())==boost::date_time::is_in_dst);
    check("check if time is dst -- ambig", 
          us_dst_calc2::local_is_dst(t7.date(),t7.time_of_day())==boost::date_time::ambiguous);
    check("check if time is dst -- ambig", 
          us_dst_calc2::local_is_dst(t8.date(),t8.time_of_day())==boost::date_time::ambiguous);
    check("check if time is dst -- not",   
          us_dst_calc2::local_is_dst(t9.date(),t9.time_of_day())==boost::date_time::is_not_in_dst);
  }
  {
    //some new checks for the new 2007 us dst rules
    date dst_start07(2007,Mar, 11);
    date dst_end07(2007,Nov, 4);

    check("dst start07", us_dst_calc2::local_dst_start_day(2007) == dst_start07);
    check("dst end07",   us_dst_calc2::local_dst_end_day(2007) == dst_end07);
    check("dst boundary07",   us_dst_calc2::is_dst_boundary_day(dst_start07));
    check("dst boundary07",   us_dst_calc2::is_dst_boundary_day(dst_end07));

    date dst_start08(2008,Mar, 9);
    date dst_end08(2008,Nov, 2);

    check("dst start08", us_dst_calc2::local_dst_start_day(2008) == dst_start08);
    check("dst end08",   us_dst_calc2::local_dst_end_day(2008) == dst_end08);
    check("dst boundary08",   us_dst_calc2::is_dst_boundary_day(dst_start08));
    check("dst boundary08",   us_dst_calc2::is_dst_boundary_day(dst_end08));

    date dst_start09(2009,Mar, 8);
    date dst_end09(2009,Nov, 1);

    check("dst start09", us_dst_calc2::local_dst_start_day(2009) == dst_start09);
    check("dst end09",   us_dst_calc2::local_dst_end_day(2009) == dst_end09);
    check("dst boundary09",   us_dst_calc2::is_dst_boundary_day(dst_start09));
    check("dst boundary09",   us_dst_calc2::is_dst_boundary_day(dst_end09));
    
  }



  /******************** post release 1 -- new dst calc engine - eu dst ********/
  

   typedef boost::date_time::eu_dst_trait<date> eu_dst_traits;
   typedef boost::date_time::dst_calc_engine<date, time_duration, eu_dst_traits>
     eu_dst_calc;
  date eu_dst_start(2002,Mar, 31);
  date eu_dst_end(2002,Oct, 27);
  ptime eu_invalid1(eu_dst_start, time_duration(2,0,0));   //invalid time label 
  ptime eu_invalid2(eu_dst_start, time_duration(2,59,59)); //invalid time label
  ptime eu_notdst1(eu_dst_start, time_duration(1,59,59));   //not ds 
  ptime eu_isdst1(eu_dst_start, time_duration(3,0,0));   //always dst
  ptime eu_isdst2(eu_dst_end, time_duration(1,59,59)); //is dst
  ptime eu_amgbig1(eu_dst_end, time_duration(2,0,0));   //ambiguous
  ptime eu_amgbig2(eu_dst_end, time_duration(2,59,59));   //ambiguous 
  ptime eu_notdst2(eu_dst_end, time_duration(3,0,0));   //always not dst

  check("eu dst start", eu_dst_calc::local_dst_start_day(2002) == eu_dst_start);
  check("eu dst end",   eu_dst_calc::local_dst_end_day(2002) == eu_dst_end);
  check("eu dst boundary",   eu_dst_calc::is_dst_boundary_day(eu_dst_start));
  check("eu dst boundary",   eu_dst_calc::is_dst_boundary_day(eu_dst_end));
  // on forward shift boundaries
  check("eu label on dst boundary invalid", 
        eu_dst_calc::local_is_dst(eu_invalid1.date(),eu_invalid1.time_of_day())==boost::date_time::invalid_time_label);
  check("eu label on dst boundary invalid",   
        eu_dst_calc::local_is_dst(eu_invalid2.date(),eu_invalid2.time_of_day())==boost::date_time::invalid_time_label);
  check("eu check if time is dst -- not",   
        eu_dst_calc::local_is_dst(eu_notdst1.date(),eu_notdst1.time_of_day())==boost::date_time::is_not_in_dst);
  check("check if time is dst -- yes",   
        eu_dst_calc::local_is_dst(eu_isdst1.date(),eu_isdst1.time_of_day())==boost::date_time::is_in_dst);
  //backward shift boundary
  check("eu check if time is dst -- yes",   
        eu_dst_calc::local_is_dst(eu_isdst2.date(),eu_isdst2.time_of_day())==boost::date_time::is_in_dst);
  check("eu check if time is dst -- ambig", 
        eu_dst_calc::local_is_dst(eu_amgbig1.date(),eu_amgbig1.time_of_day())==boost::date_time::ambiguous);
  check("eu check if time is dst -- ambig", 
        eu_dst_calc::local_is_dst(eu_amgbig2.date(),eu_amgbig2.time_of_day())==boost::date_time::ambiguous);
  check("eu check if time is dst -- not",   
        eu_dst_calc::local_is_dst(eu_notdst2.date(),eu_notdst2.time_of_day())==boost::date_time::is_not_in_dst);
  
/******************** post release 1 -- new dst calc engine - gb dst ********/

  
  /* Several places in Great Britan use eu start and end rules for the 
     day, but different local conversion times (eg: forward change at 1:00 
     am local and  backward change at 2:00 am dst instead of 2:00am 
     forward and 3:00am back for the EU).
  */
  
  typedef boost::date_time::uk_dst_trait<date> uk_dst_traits;

  typedef boost::date_time::dst_calc_engine<date, time_duration, uk_dst_traits>  uk_dst_calc;


  date uk_dst_start(2002,Mar, 31);
  date uk_dst_end(2002,Oct, 27);
  ptime uk_invalid1(uk_dst_start, time_duration(1,0,0));   //invalid time label 
  ptime uk_invalid2(uk_dst_start, time_duration(1,59,59)); //invalid time label
  ptime uk_notdst1(uk_dst_start, time_duration(0,59,59));   //not ds 
  ptime uk_isdst1(uk_dst_start, time_duration(2,0,0));   //always dst
  ptime uk_isdst2(uk_dst_end, time_duration(0,59,59)); //is dst
  ptime uk_amgbig1(uk_dst_end, time_duration(1,0,0));   //ambiguous
  ptime uk_amgbig2(uk_dst_end, time_duration(1,59,59));   //ambiguous 
  ptime uk_notdst2(uk_dst_end, time_duration(3,0,0));   //always not dst

  check("uk dst start", uk_dst_calc::local_dst_start_day(2002) == uk_dst_start);
  check("uk dst end",   uk_dst_calc::local_dst_end_day(2002) == uk_dst_end);
  check("uk dst boundary",   uk_dst_calc::is_dst_boundary_day(uk_dst_start));
  check("uk dst boundary",   uk_dst_calc::is_dst_boundary_day(uk_dst_end));
  // on forward shift boundaries
  check("uk label on dst boundary invalid", 
        uk_dst_calc::local_is_dst(uk_invalid1.date(),uk_invalid1.time_of_day())==boost::date_time::invalid_time_label);
  check("uk label on dst boundary invalid",   
        uk_dst_calc::local_is_dst(uk_invalid2.date(),uk_invalid2.time_of_day())==boost::date_time::invalid_time_label);
  check("uk check if time is dst -- not",   
        uk_dst_calc::local_is_dst(uk_notdst1.date(),uk_notdst1.time_of_day())==boost::date_time::is_not_in_dst);
  check("uk check if time is dst -- yes",   
        uk_dst_calc::local_is_dst(uk_isdst1.date(),uk_isdst1.time_of_day())==boost::date_time::is_in_dst);
  //backward shift boundary
  check("uk check if time is dst -- yes",   
        uk_dst_calc::local_is_dst(uk_isdst2.date(),uk_isdst2.time_of_day())==boost::date_time::is_in_dst);
  check("uk check if time is dst -- ambig", 
        uk_dst_calc::local_is_dst(uk_amgbig1.date(),uk_amgbig1.time_of_day())==boost::date_time::ambiguous);
  check("uk check if time is dst -- ambig", 
        uk_dst_calc::local_is_dst(uk_amgbig2.date(),uk_amgbig2.time_of_day())==boost::date_time::ambiguous);
  check("uk check if time is dst -- not",   
        uk_dst_calc::local_is_dst(uk_notdst2.date(),uk_notdst2.time_of_day())==boost::date_time::is_not_in_dst);


//   /******************** post release 1 -- new dst calc engine ********/

//   //Define dst rule for Paraguay which is transitions forward on Oct 1 and back Mar 1
  
  typedef boost::date_time::dst_calc_engine<date, time_duration, 
                                            paraguay_dst_traits>  pg_dst_calc;

  {

    date pg_dst_start(2002,Oct, 1);
    date pg_dst_end(2002,Mar, 1);
    date pg_indst(2002,Dec, 1);
    date pg_notdst(2002,Jul, 1);
    ptime pg_invalid1(pg_dst_start, time_duration(2,0,0));   //invalid time label 
    ptime pg_invalid2(pg_dst_start, time_duration(2,59,59)); //invalid time label
    ptime pg_notdst1(pg_dst_start, time_duration(1,59,59));   //not ds 
    ptime pg_isdst1(pg_dst_start, time_duration(3,0,0));   //always dst
    ptime pg_isdst2(pg_dst_end, time_duration(0,59,59)); //is dst
    ptime pg_amgbig1(pg_dst_end, time_duration(1,0,0));   //ambiguous
    ptime pg_amgbig2(pg_dst_end, time_duration(1,59,59));   //ambiguous 
    ptime pg_notdst2(pg_dst_end, time_duration(2,0,0));   //always not dst
    
    check("pg dst start", pg_dst_calc::local_dst_start_day(2002) == pg_dst_start);
    check("pg dst end",   pg_dst_calc::local_dst_end_day(2002) == pg_dst_end);
    check("pg dst boundary",   pg_dst_calc::is_dst_boundary_day(pg_dst_start));
    check("pg dst boundary",   pg_dst_calc::is_dst_boundary_day(pg_dst_end));
    // on forward shift boundaries
    check("pg label on dst boundary invalid", 
          pg_dst_calc::local_is_dst(pg_invalid1.date(),pg_invalid1.time_of_day())==boost::date_time::invalid_time_label);
    check("pg label on dst boundary invalid",   
          pg_dst_calc::local_is_dst(pg_invalid2.date(),pg_invalid2.time_of_day())==boost::date_time::invalid_time_label);
    check("pg check if time is dst -- not",   
          pg_dst_calc::local_is_dst(pg_notdst1.date(),pg_notdst1.time_of_day())==boost::date_time::is_not_in_dst);
    check("check if time is dst -- yes",   
          pg_dst_calc::local_is_dst(pg_isdst1.date(),pg_isdst1.time_of_day())==boost::date_time::is_in_dst);
    //backward shift boundary
    check("pg check if time is dst -- yes",   
          pg_dst_calc::local_is_dst(pg_isdst2.date(),pg_isdst2.time_of_day())==boost::date_time::is_in_dst);
    check("pg check if time is dst -- ambig", 
          pg_dst_calc::local_is_dst(pg_amgbig1.date(),pg_amgbig1.time_of_day())==boost::date_time::ambiguous);
    check("pg check if time is dst -- ambig", 
          pg_dst_calc::local_is_dst(pg_amgbig2.date(),pg_amgbig2.time_of_day())==boost::date_time::ambiguous);
    check("pg check if time is dst -- not",   
          pg_dst_calc::local_is_dst(pg_notdst2.date(),pg_notdst2.time_of_day())==boost::date_time::is_not_in_dst);
    // a couple not on the boudnary
    check("pg check if time is dst -- yes",   
          pg_dst_calc::local_is_dst(pg_indst,time_duration(0,0,0))==boost::date_time::is_in_dst);
    check("pg check if time is dst -- not",   
          pg_dst_calc::local_is_dst(pg_notdst,time_duration(0,0,0))==boost::date_time::is_not_in_dst);
    
  }

//   /******************** post release 1 -- new dst calc engine ********/

//   //Define dst rule for Adelaide australia
  
  typedef boost::date_time::acst_dst_trait<date> acst_dst_traits;
  typedef boost::date_time::dst_calc_engine<date, time_duration, 
                                            acst_dst_traits>  acst_dst_calc;

  {

    date acst_dst_start(2002,Oct, 27);
    date acst_dst_end(2002,Mar, 31);
    date acst_indst(2002,Dec, 1);
    date acst_notdst(2002,Jul, 1);
    ptime acst_invalid1(acst_dst_start, time_duration(2,0,0));   //invalid time label 
    ptime acst_invalid2(acst_dst_start, time_duration(2,59,59)); //invalid time label
    ptime acst_notdst1(acst_dst_start, time_duration(1,59,59));   //not ds 
    ptime acst_isdst1(acst_dst_start, time_duration(3,0,0));   //always dst
    ptime acst_isdst2(acst_dst_end, time_duration(1,59,59)); //is dst
    ptime acst_amgbig1(acst_dst_end, time_duration(2,0,0));   //ambiguous
    ptime acst_amgbig2(acst_dst_end, time_duration(2,59,59));   //ambiguous 
    ptime acst_notdst2(acst_dst_end, time_duration(3,0,0));   //always not dst
    
//     std::cout << "acst dst_start: " << acst_dst_calc::local_dst_start_day(2002)
//               << std::endl;
    check("acst dst start", acst_dst_calc::local_dst_start_day(2002) == acst_dst_start);
    check("acst dst end",   acst_dst_calc::local_dst_end_day(2002) == acst_dst_end);
    check("acst dst boundary",   acst_dst_calc::is_dst_boundary_day(acst_dst_start));
    check("acst dst boundary",   acst_dst_calc::is_dst_boundary_day(acst_dst_end));
    // on forward shift boundaries
    check("acst label on dst boundary invalid", 
          acst_dst_calc::local_is_dst(acst_invalid1.date(),acst_invalid1.time_of_day())==boost::date_time::invalid_time_label);
    check("acst label on dst boundary invalid",   
          acst_dst_calc::local_is_dst(acst_invalid2.date(),acst_invalid2.time_of_day())==boost::date_time::invalid_time_label);
    check("acst check if time is dst -- not",   
          acst_dst_calc::local_is_dst(acst_notdst1.date(),acst_notdst1.time_of_day())==boost::date_time::is_not_in_dst);
    check("check if time is dst -- yes",   
          acst_dst_calc::local_is_dst(acst_isdst1.date(),acst_isdst1.time_of_day())==boost::date_time::is_in_dst);
    //backward shift boundary
    check("acst check if time is dst -- yes",   
          acst_dst_calc::local_is_dst(acst_isdst2.date(),acst_isdst2.time_of_day())==boost::date_time::is_in_dst);
    check("acst check if time is dst -- ambig", 
          acst_dst_calc::local_is_dst(acst_amgbig1.date(),acst_amgbig1.time_of_day())==boost::date_time::ambiguous);
    check("acst check if time is dst -- ambig", 
          acst_dst_calc::local_is_dst(acst_amgbig2.date(),acst_amgbig2.time_of_day())==boost::date_time::ambiguous);
    check("acst check if time is dst -- not",   
          acst_dst_calc::local_is_dst(acst_notdst2.date(),acst_notdst2.time_of_day())==boost::date_time::is_not_in_dst);
    // a couple not on the boudnary
    check("acst check if time is dst -- yes",   
          acst_dst_calc::local_is_dst(acst_indst,time_duration(0,0,0))==boost::date_time::is_in_dst);
    check("acst check if time is dst -- not",   
          acst_dst_calc::local_is_dst(acst_notdst,time_duration(0,0,0))==boost::date_time::is_not_in_dst);
    //    ptime utc_t = ptime(acst_dst_start, hours(2)) - time_duration(16,30,0);
    //    std::cout << "UTC date/time of Adelaide switch over: " << utc_t << std::endl;
    
  }

  return printTestStats();

}

