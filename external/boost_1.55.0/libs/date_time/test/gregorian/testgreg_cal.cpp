/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/greg_calendar.hpp"
#include "../testfrmwk.hpp"
#include <iostream>


void
test_greg_calendar()
{
  using namespace boost::gregorian;
  check("Day of week 2000-09-24 == 0 (Sun)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2000,9,24))==0);
  check("Day of week 2000-09-25 == 1 (Mon)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2000,9,25))==1);
  check("Day of week 2000-09-26 == 2 (Tue)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2000,9,26))==2);
  check("Day of week 2000-09-27 == 3 (Wed)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2000,9,27))==3);
  check("Day of week 2000-09-28 == 4 (Thu)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2000,9,28))==4);
  check("Day of week 2000-09-29 == 5 (Fri)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2000,9,29))==5);
  check("Day of week 2000-09-30 == 6 (Sat)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2000,9,30))==6);
  //see calendar FAQ 2.2 for reference
  check("Day of week 1953-08-02 == 0 (Sun)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1953,8,2))==0);
  check("Day of week 1953-08-03 == 1 (Mon)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1953,8,3))==1);
  check("Day of week 1953-08-04 == 2 (Tue)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1953,8,4))==2);
  check("Day of week 1953-08-05 == 3 (Wed)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1953,8,5))==3);
  check("Day of week 1953-08-06 == 4 (Thu)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1953,8,6))==4);
  check("Day of week 1953-08-07 == 5 (Fri)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1953,8,7))==5);
  check("Day of week 1953-08-08 == 6 (Sat)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1953,8,8))==6);
  check("Day of week 2001-08-31 == 5 (Fri)", gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2001,8,31))==5);
  
  //Checked against Caledrical Calc M. Edition p 396 and www site
  check("Day of week 1400-01-01 == 3 (Wed)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1400,1,1))==3);
  check("Day of week 1436-02-03 == 3 (Wed)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1436,2,3))==3);
  check("Day of week 1492-04-9 == 6 (Sat)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1492,4,9))==6);
  check("Day of week 1560-03-5 == 6 (Sat)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1560,3,5))==6);
  check("Day of week 1716-07-24 == 5 (Fri)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1716,7,24))==5);
  check("Day of week 1768-06-19 == 0 (Sun)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1768,6,19))==0);
  check("Day of week 1839-03-27 == 3 (Wed)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1839,3,27))==3);
  check("Day of week 1819-08-02 == 1 (Mon)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1819,8,2))==1);
  check("Day of week 1903-04-19 == 0 (Sun)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1903,4,19))==0);
  check("Day of week 1929-08-25 == 0 (Sun)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(1929,8,25))==0);
  check("Day of week 2038-11-10 == 3 (Wed)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2038,11,10))==3);
  check("Day of week 2094-07-18 == 0 (Sun)", 
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(2094,7,18))==0);
  //verified against website applet
  check("Day of week 3002-07-10 == 6 (Sat)",
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(3002,7,10))==6);
  //verified against website applet
  check("Day of week 4002-07-10 == 3 (Wed)",
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(4002,7,10))==3);
  //verified against website applet
  check("Day of week 5002-07-10 == 6 (Sat)",
        gregorian_calendar::day_of_week(gregorian_calendar::ymd_type(5002,7,10))==6);

  check("1404 is a leap year", gregorian_calendar::is_leap_year(1404));
  check("2000 is a leap year", gregorian_calendar::is_leap_year(2000));
  check("2004 is a leap year", gregorian_calendar::is_leap_year(2004));
  check("2400 is a leap year", gregorian_calendar::is_leap_year(2400));
  check("4000 is a leap year", gregorian_calendar::is_leap_year(4000));
  check("1400 is NOT a leap year", !gregorian_calendar::is_leap_year(1400));
  check("1900 is NOT a leap year", !gregorian_calendar::is_leap_year(1900));
  check("2100 is NOT a leap year", !gregorian_calendar::is_leap_year(2100));
  check("1999 is NOT a leap year", !gregorian_calendar::is_leap_year(1999));
  check("5000 is NOT a leap year", !gregorian_calendar::is_leap_year(5000));


  unsigned long jday1 = gregorian_calendar::day_number(gregorian_calendar::ymd_type(2000,1,1));
  unsigned long jday2 = gregorian_calendar::day_number(gregorian_calendar::ymd_type(2001,1,1));
//   unsigned short year, month, day;
//   //2451545 is 2000-1-1
  check("ToDayNumber 2000-1-1 is day 2451545", jday1 == 2451545);
  check("ToDayNumber 2001-1-1 is day 2451911", jday2 == 2451911);
  gregorian_calendar::ymd_type ymd = gregorian_calendar::from_day_number(jday1);
  check("from_day_number test 2000-1-1",  (ymd.year==2000)&&
                                          (ymd.month==1) &&
                                          (ymd.day==1) );

  unsigned long jday3 = gregorian_calendar::day_number(gregorian_calendar::ymd_type(1999,1,1));
  check("366 days between 2000-1-1 and 2001-1-1", (jday2-jday1) == 366);
  check("731 days between 1999-1-1 and 2001-1-1 ",(jday2-jday3) == 731);

  unsigned long jday4 = gregorian_calendar::day_number(gregorian_calendar::ymd_type(2000,2,28));
  unsigned long jday5 = gregorian_calendar::day_number(gregorian_calendar::ymd_type(2000,3,1));
  check("2 days between 2000-2-28 and 2000-3-1 ",(jday5-jday4) == 2);

  check("31 days in month Jan 2000", gregorian_calendar::end_of_month_day(2000,1) == 31);
  check("29 days in month Feb 2000", gregorian_calendar::end_of_month_day(2000,2) == 29);
  check("28 days in month Feb 1999", gregorian_calendar::end_of_month_day(1999,2) == 28);
  check("28 days in month Feb 2001", gregorian_calendar::end_of_month_day(2001,2) == 28);
  check("31 days in month Mar 2000", gregorian_calendar::end_of_month_day(2000,3) == 31);
  check("30 days in month Apr 2000", gregorian_calendar::end_of_month_day(2000,4) == 30);
  check("31 days in month May 2000", gregorian_calendar::end_of_month_day(2000,5) == 31);
  check("30 days in month Jun 2000", gregorian_calendar::end_of_month_day(2000,6) == 30);
  check("31 days in month Jul 2000", gregorian_calendar::end_of_month_day(2000,7) == 31);
  check("31 days in month Aug 2000", gregorian_calendar::end_of_month_day(2000,8) == 31);
  check("30 days in month Sep 2000", gregorian_calendar::end_of_month_day(2000,9) == 30);
  check("31 days in month Oct 2000", gregorian_calendar::end_of_month_day(2000,10) == 31);
  check("30 days in month Nov 2000", gregorian_calendar::end_of_month_day(2000,11) == 30);
  check("31 days in month Dec 2000", gregorian_calendar::end_of_month_day(2000,12) == 31);


//   unsigned long jday0 = date_limits<>::dayNumber(gregorian_calendar::ymd_type(1900,1,1));
//   std::cout << "Day 0: " << jday0 << std::endl;

  std::cout << gregorian_calendar::epoch().year << std::endl;
}


int
main() 
{
  test_greg_calendar();
  return printTestStats();
}

