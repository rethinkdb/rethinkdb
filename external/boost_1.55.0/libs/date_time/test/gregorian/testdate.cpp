/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include <iostream>
#include <boost/cstdint.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"

int
main()
{

  using namespace boost::gregorian;

  //various constructors
#if !defined(DATE_TIME_NO_DEFAULT_CONSTRUCTOR)
  date def;
  check("Default constructor", def == date(not_a_date_time));
#endif
  date d(2000,1,1);
  date d1(1900,1,1);
  date d2 = d;
  date d3(d2);
  check("Copy constructor", d3 == d2);
  date d4(2000,12,31);
  date d4a(2000,Dec,31);
  //d4a.print(std::cout); std::cout << std::endl;
  check("month_rep constructor",   d4 == d4a);
  //std::cout << d3 << std::endl;
  //retrieval functions
  check_equal("1900-01-01 day is 01",     d1.day(),   1);
  check_equal("1900-01-01 month is 01",   d1.month(), 1);
  check_equal("1900-01-01 year is 1900",  d1.year(),  1900);
  check_equal("2000-12-31 day is 31",     d4.day(),   31);
  check_equal("2000-12-31 month is 12",   d4.month(), 12);
  check_equal("2000-12-31 year is 2000",  d4.year(),  2000);
  //operator<
  check("1900-01-01 is less than 2000-01-01",          d1 < d2);
  check("2000-01-01 is NOT less than 2000-01-01",      !(d1 < d1));
  //operator<=
  check("2000-01-01 is less equal than 2000-01-01",    d1 <= d1);
  //operator>
  check("2000-01-01 is greater than 1900-01-01",       d2 > d1);
  check("2000-01-01 is NOT greater than 2000-01-01",   !(d1 < d1));
  //operator>=
  check("2000-01-01 is greater equal than 2000-01-01", d1 >= d1);
  //operator!=
  check("2000-01-01 is NOT equal to 1900-01-01",       d2 != d1);
  //operator==
  check_equal("2000-01-01 is equal 2000-01-01",        d3,   d2);
  check("2000-01-01 is greater equal 2000-01-01",      d3 >= d2);
  check("2000-01-01 is greater equal 2000-01-01",      d3 <= d2);

  date::ymd_type ymd = d1.year_month_day();
  check_equal("ymd year",  ymd.year,  1900);
  check_equal("ymd month", ymd.month, 1);
  check_equal("ymd day",   ymd.day,   1);

  //The max function will not compile with Borland 5.5
  //Complains about must specialize basic_data<limits> ???
//   std::cout << "Max date is " << (date::max)() << std::endl;
//   //std::cout << "Max date is " << (basic_date< date_limits<unsigned int,1900> >::max)() << std::endl;
//   //std::cout << "Max date is " << (date_limits<unsigned int, 1900>::max)() << std::endl;

  const date answers[] = {date(1900,Jan,1),date(1900,Jan,4),date(1900,Jan,7),
                          date(1900,Jan,10),date(1900,Jan,13)};
  date_duration off(3);
  date d5(1900,1,1);
  for (int i=0; i < 5; ++i) {
    //std::cout << d5 << "  ";
    check(" addition ", d5 == answers[i]);
    d5 = d5 + off;
  }
  std::cout << std::endl;

   const date answers1[] = {date(2000,2,26),date(2000,2,28),date(2000,Mar,1)};
   date d8(2000,Feb,26);
   for (int j=0; j < 3; ++j) {
     //std::cout << d8 << "  ";
     check(" more addition ", d8 == answers1[j]);
     d8 = d8 + days(2);
   }
   // std::cout << std::endl;

  date d6(2000,2,28);
  date d7(2000,3,1);
  date_duration twoDays(2);
  date_duration negtwoDays(-2);
  date_duration zeroDays(0);
  check_equal("2000-03-01 - 2000-02-28 == 2 days",   twoDays,     (d7-d6));
  check_equal("2000-02-28 - 2000-03-01 == - 2 days", negtwoDays,  (d6-d7));
  check_equal("2000-02-28 - 2000-02-28 == 0 days",   zeroDays,    (d6-d6));
  check_equal("2000-02-28 + 2 days == 2000-03-01 ",  d6 + twoDays, d7);
  check_equal("2000-03-01 - 2 days == 2000-02-28 ",  d7 - twoDays, d6);
  check_equal("Add duration to date", date(1999,1,1) + date_duration(365), date(2000,1,1));
  check_equal("Add zero days", date(1999,1,1) + zeroDays, date(1999,1,1));
  //can't do this...
  //check("Add date to duration", date_duration(365) + date(1999,1,1) == date(2000,1,1));

  {
    date d(2003,Oct,31);
    date_duration dd(55);
    d += dd;
    check("date += date_duration", d == date(2003,Dec,25));
    d -= dd;
    check("date -= date_duration", d == date(2003,Oct,31));
    /* special_values is more thoroughly tested later,
     * this is just a test of += & -= with special values */
    d += date_duration(pos_infin);
    check("date += inf_dur", d == date(pos_infin));
    d -= dd;
    check("inf_date -= dur", d == date(pos_infin));
  }
  {
    date d(2003,Oct,31);
    date_duration dd1(pos_infin), dd2(neg_infin), dd3(not_a_date_time);
    check_equal("date + inf_dur", d + dd1, date(pos_infin));
    check_equal("date + inf_dur", d + dd2, date(neg_infin));
    check_equal("date + nan_dur", d + dd3, date(not_a_date_time));
    check_equal("date - inf_dur", d - dd1, date(neg_infin));
    check_equal("date - inf_dur", d - dd2, date(pos_infin));
    check_equal("date - nan_dur", d - dd3, date(not_a_date_time));
    check_equal("inf_date + inf_dur", date(pos_infin) + dd1, date(pos_infin));
    check_equal("inf_date - inf_dur", date(pos_infin) - dd1, date(not_a_date_time));
    check_equal("inf_date + inf_dur", date(neg_infin) + dd1, date(not_a_date_time));
    check_equal("inf_date - inf_dur", date(neg_infin) - dd1, date(neg_infin));
  }


  try {
    date d9(2000, Jan, 32);
    check("day out of range", false);
    //never reached if working -- but stops compiler warnings :-)
    std::cout << "Oops: " << to_iso_string(d9) << std::endl;
  }
  catch (bad_day_of_month&) {
    check("day out of range", true);
  }
  try {
    date d9(2000, Jan, 0);
    check("day out of range", false);
    //never reached if working -- but stops compiler warnings :-)
    std::cout << "Oops: " << to_iso_string(d9) << std::endl;
  }
  catch (bad_day_of_month&) {
    check("day out of range", true);
  }

  try {
    date d20(2000, Feb, 31);
    check("day out of range", false);
    //never reached if working -- but stops compiler warnings :-)
    std::cout << "Oops: " << to_iso_string(d20) << std::endl;
  }
  catch (bad_day_of_month&) {
    check("day out of range", true);
  }

  //more subtle -- one day past in a leap year
  try {
    date d21(2000, Feb, 30);
    check("day out of range", false);
    //never reached if working -- but stops compiler warnings :-)
    std::cout << "Oops: " << to_iso_string(d21) << std::endl;
  }
  catch (bad_day_of_month&) {
    check("day out of range", true);
  }

  //more subtle -- one day past in a leap year
  try {
    date d22(2000, Feb, 29);
    check("last day of month ok", true);
    std::cout << to_iso_string(d22) << std::endl; //stop compiler warning
  }
  catch (bad_day_of_month&) {
    check("last day of month -- oops bad exception", false);
  }

  //Not a leap year -- now Feb 29 is bad
  try {
    date d23(1999, Feb, 29);
    check("day out of range", false);
    //never reached if working -- but stops compiler warnings :-)
    std::cout << "Oops: " << to_iso_string(d23) << std::endl;
  }
  catch (bad_day_of_month&) {
    check("day out of range", true);
  }

  //check out some special values
  check("check not a date - false",           !d7.is_not_a_date());
  check("check positive infinity - false",    !d7.is_pos_infinity());
  check("check negative infinity - false",    !d7.is_neg_infinity());

  date d10(neg_infin);
  check("check negative infinity - true",     d10.is_infinity());
  d10 = d10 + twoDays; //still neg infinity
  check("check negative infinity - true",     d10.is_neg_infinity());

  date d11(pos_infin);
  check("check positive infinity - true",     d11.is_infinity());
  d11 = d11 + twoDays;
  check("check positive infinity add - true", d11.is_pos_infinity());

  date d12(not_a_date_time);
  check("check not a date",                   d12.is_not_a_date());
  check("check infinity compare   ",          d10 != d11);
  check("check infinity compare   ",          d10 < d11);
  check("check infinity nad compare   ",      d12 != d11);
  date d13(max_date_time);
  check("check infinity - max compare   ",      d13 < d11);
  check_equal("max date_time value   ",       d13, date(9999,Dec, 31));
  std::cout << to_simple_string(d13) << std::endl;
  date d14(min_date_time);
  check("check infinity - min compare   ",      d14 > d10);
  std::cout << to_simple_string(d14) << std::endl;
  check_equal("min date_time value   ",      d14, date(1400,Jan, 1));


  date d15(1400,1,1);
  std::cout << d15.day_of_week().as_long_string() << std::endl;
  check("check infinity - min compare   ",      d10 < d15);

  // most of this testing is in the gregorian_calendar tests
  std::cout << d15.julian_day() << std::endl;
  check_equal("check julian day   ", d15.julian_day(), 
      static_cast<boost::uint32_t>(2232400));
  check_equal("check modjulian day   ", d15.modjulian_day(), -167601);
  date d16(2004,2,29);
  check_equal("check julian day   ", d16.julian_day(), 
      static_cast<boost::uint32_t>(2453065));
  check_equal("check modjulian day   ", d16.modjulian_day(), 
      static_cast<boost::uint32_t>(53064));

  // most of this testing is in the gregorian_calendar tests
  date d31(2000, Jun, 1);
  check_equal("check iso week number   ", d31.week_number(), 22);
  date d32(2000, Aug, 1);
  check_equal("check iso week number   ", d32.week_number(), 31);
  date d33(2000, Oct, 1);
  check_equal("check iso week number   ", d33.week_number(), 39);
  date d34(2000, Dec, 1);
  check_equal("check iso week number   ", d34.week_number(), 48);
  date d35(2000, Dec, 24);
  check_equal("check iso week number   ", d35.week_number(), 51);
  date d36(2000, Dec, 25);
  check_equal("check iso week number   ", d36.week_number(), 52);
  date d37(2000, Dec, 31);
  check_equal("check iso week number   ", d37.week_number(), 52);
  date d38(2001, Jan, 1);
  check_equal("check iso week number   ", d38.week_number(), 1);

  try {
    int dayofyear1 = d38.day_of_year();
    check_equal("check day of year number", dayofyear1, 1);
    check_equal("check day of year number", d37.day_of_year(), 366);
    date d39(2001,Dec,31);
    check_equal("check day of year number", d39.day_of_year(), 365);
    date d40(2000,Feb,29);
    check_equal("check day of year number", d40.day_of_year(), 60);
    date d41(1400,Jan,1);
    check_equal("check day of year number", d41.day_of_year(), 1);
    date d42(1400,Jan,1);
    check_equal("check day of year number", d42.day_of_year(), 1);
    date d43(2002,Nov,17);
    check_equal("check day of year number", d43.day_of_year(), 321);
  }
  catch(std::exception& e) {
    std::cout << e.what() << std::endl;
    check("check day of year number", false);
  }

  //converts to date and back -- should get same result
  check_equal("tm conversion functions 2000-1-1", date_from_tm(to_tm(d)), d);
  check_equal("tm conversion functions 1900-1-1", date_from_tm(to_tm(d1)), d1);
  check_equal("tm conversion functions min date 1400-1-1 ", date_from_tm(to_tm(d14)), d14);
  check_equal("tm conversion functions max date 9999-12-31", date_from_tm(to_tm(d13)), d13);

  try{
    date d(neg_infin);
    tm d_tm = to_tm(d);
    check("Exception not thrown (special_value to_tm)", false);
    std::cout << d_tm.tm_sec << std::endl; //does nothing useful but stops compiler from complaining about unused d_tm
  }catch(std::out_of_range& e){
    check("Caught expected exception (special_value to_tm)", true);
  }catch(...){
    check("Caught un-expected exception (special_value to_tm)", false);
  }

  return printTestStats();

}

