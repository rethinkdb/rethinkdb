/* Copyright (c) 2002-2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2012-10-10 12:05:03 -0700 (Wed, 10 Oct 2012) $
 */

#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "boost/date_time/compiler_config.hpp"
#if defined(BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS)
#include "boost/date_time/posix_time/time_formatters_limited.hpp"
#else
#include "boost/date_time/posix_time/time_formatters.hpp"
#endif
#include "../testfrmwk.hpp"


int
main() 
{
  using namespace boost::posix_time;

  
//   std::cout << "Default limits: SECOND " << std::endl;
  {
    time_duration td;
    check("default construction -- 0 ticks", td.ticks() == 0);
//     check("default construction -- 0 secs", td.seconds() == 0);
//     check("default construction -- 0 min", td.minutes() == 0); 
  }
  
  // construct from components
  time_duration td1(1,25,0);
  time_duration td3(td1.hours(),td1.minutes(),td1.seconds());
  check("total up elements", td1 == td3);
  td1 = -td1; // td1 == "-1:25:00"
  td3 = time_duration(td1.hours(),td1.minutes(),td1.seconds());
  check("total up elements-invered sign", td1 == td3);
  
  
  time_duration t_1(0,1,40);
  time_duration t_2(0,1,41);
  check("less test", !(t_2 < t_2));
  check("equal test", t_1 == t_1);
  check("greater equal - equal", t_1 >= t_1);
  check("greater equal - greater", t_2 >= t_1);
  check("less equal - equal  ", t_2 <= t_2);
  check("greater             ", t_2 > t_1);
  check("greater - not       ", !(t_1 > t_2));
  time_duration t_3(t_2);
  check("copy constructor    ", t_2 == t_3);
  time_duration t_4 = t_3;
  check("assignment operator ", t_2 == t_4);

  time_duration t_5(1,30,20,10); // 1hr, 30min, 20sec, 10 frac sec
  t_5 /= 2;
  check("divide equal",         (t_5.hours() == 0 &&
                                 t_5.minutes() == 45 &&
                                 t_5.seconds() == 10 &&
                                 t_5.fractional_seconds() == 5));
  t_5 = time_duration(3,15,8,0) / 2;
  check("divide int", t_5 == time_duration(1,37,34,0));
  {
    time_duration td = hours(5);
    td *= 5;
    check("mult-equals int", time_duration(25,0,0,0) == td);
  }
 
  
  t_5 = t_2 + t_1;
  //VC6 goes ambiguous on the next line...
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
  //sorry ticks() doesn't work on VC6
#else
  std::cout << t_5.ticks() << std::endl;  
#endif
  check("add", t_5 == time_duration(0,3,21));
  time_duration td_a(5,5,5,5);
  time_duration td_b(4,4,4,4);
  time_duration td_c(2,2,2,2);
  td_a += td_b;
  check("add equal", td_a == time_duration(9,9,9,9));
  time_duration td_d = td_b - td_c; 
  check("subtract", td_d == time_duration(2,2,2,2));
  td_d -= td_b; 
  check("subtract equal (neg result)", td_d == td_c - td_b);

  time_duration utd(1,2,3,4);
  time_duration utd2 = -utd;
  //std::cout << td_d << '\n' << utd2 << std::endl;
  check("unary-", ((utd2.hours() == -1) &&
                   (utd2.minutes() == -2) &&
                   (utd2.seconds() == -3) &&
                   (utd2.fractional_seconds() == -4)) );
  utd2 = -hours(5);
  check("unary-", utd2.hours() == -5);
  utd2 = -utd2;
  check("unary-", utd2.hours() == 5);

  time_duration t_6(5,4,3); //05:04:03
  check("h-m-s 5-4-3 hours", t_6.hours() == 5);
  check("h-m-s 5-4-3 minutes", t_6.minutes() == 4);
  check("h-m-s 5-4-3 seconds", t_6.seconds() == 3);
  std::cout << t_6.total_seconds() << std::endl;
  check("h-m-s 5-4-3 total_seconds", t_6.total_seconds() == 18243);

  hours tenhours(10);
  minutes fivemin(5);
  time_duration t7 = time_duration(1,2,3) + tenhours + fivemin;
  check("short hand durations add", t7 == time_duration(11,7,3));
  time_duration t8 = tenhours + time_duration(1,2,3) +  fivemin;
  check("short hand durations add", t8 == time_duration(11,7,3));

  if (time_duration::resolution() >= boost::date_time::micro) {
    time_duration t_9(5,4,3,9876); //05:04:03.09876
    check("h-m-s 5-4-3.21 hours",  t_9.hours() == 5);
    check("h-m-s 5-4-3.21 min  ",  t_9.minutes() == 4);
    check("h-m-s 5-4-3.21 sec  ",  t_9.seconds() == 3);
    check("h-m-s 5-4-3.21 fs   ",  t_9.fractional_seconds() == 9876);
    check("h-m-s 5-4-3.21 total_seconds", t_9.total_seconds() == 18243);
    //  check("h-m-s 5-4-3.21 fs   ",  t_9.fs_as_double() == 0.9876);
    //std::cout << t_9.fs_as_double() << std::endl;
    std::cout << to_simple_string(t_9) << std::endl;
  }

  if (time_duration::resolution() >= boost::date_time::tenth) {
    time_duration t_10(5,4,3,9); //05:04:03.00001
    check("h-m-s 5-4-3.9 hours",  t_10.hours() == 5);
    check("h-m-s 5-4-3.9 min  ",  t_10.minutes() == 4);
    check("h-m-s 5-4-3.9 sec  ",  t_10.seconds() == 3);
    check("h-m-s 5-4-3.9 fs   ",  t_10.fractional_seconds() == 9);
    check("h-m-s 5-4-3.9 total_seconds", t_10.total_seconds() == 18243);
    std::cout << to_simple_string(t_10) << std::endl;
  }

  if (time_duration::resolution() >= boost::date_time::milli) {
    millisec ms(9);
    //  time_duration t_10(0,0,0,); //00:00:00.009
    std::cout << "time_resolution: " << time_duration::resolution() << std::endl;
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
  //sorry res_adjust() doesn't work on VC6
#else
    std::cout << "res_adjust " << time_res_traits::res_adjust() << std::endl;
#endif
    if (time_duration::resolution() == boost::date_time::nano) {
      check("millisec",  ms.fractional_seconds() == 9000000);
      check("total_seconds - nofrac", ms.total_seconds() == 0);
      check("total_millisec", ms.total_milliseconds() == 9);
      check("ticks per second", time_duration::ticks_per_second() == 1000000000);
    }
    else {
      check("millisec 9000",  ms.fractional_seconds() == 9000);
      check("total_seconds - nofrac", ms.total_seconds() == 0);
      check("total_millisec", ms.total_milliseconds() == 9);
    }
  }

#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  if (time_duration::resolution() >= boost::date_time::nano) {
    nanosec ns(9);
    //  time_duration t_10(0,0,0,); //00:00:00.00009
    check("nanosec",  ns.fractional_seconds() == 9);
    check("total nanosec",  ns.total_nanoseconds() == 9);
    check("total microsec - truncated",  ns.total_microseconds() == 0);
    std::cout << to_simple_string(ns) << std::endl;
    time_duration ns18 = ns + ns;
    check("nanosec",  ns18.fractional_seconds() == 18);
    std::cout << to_simple_string(ns18) << std::endl;
    nanosec ns2(1000000000); //one second
    check("nano to second compare",  ns2 == seconds(1));
    check("check total seconds",  ns2.total_seconds() == 1);
    std::cout << to_simple_string(ns2) << std::endl;
    check("division of nanoseconds", (nanosec(3)/2) == nanosec(1));
    check("multiplication of nanosec", nanosec(3)*1000 == microsec(3));
  }
#endif  

  // Test for overflows (ticket #3471)
  {
    ptime start(boost::gregorian::date(2000, 1, 1));
    ptime end(boost::gregorian::date(2000, 5, 1));
    time_duration td = end - start;
    ptime end2 = start + microseconds(td.total_microseconds());
    check("microseconds constructor overflow", end == end2);
  }

  time_duration t_11(3600,0,0);
  check("3600 hours   ",  t_11.hours() == 3600);
  check("total seconds 3600 hours",  t_11.total_seconds() == 12960000);

  time_duration td_12(1,2,3,10); 
  std::cout << td_12.total_seconds() << std::endl;
  check("total seconds 3723 hours",  td_12.total_seconds() == 3723);

  //  time_duration t_11a = t_11/time_duration(0,3,0);

  check("division", (hours(2)/2) == hours(1));
  check("division", (hours(3)/2) == time_duration(1,30,0));
  check("division", (hours(3)/3) == hours(1));
  check("multiplication", time_duration(3,0,0)*2 == hours(6));
  check("multiplication", hours(3600)*1000 == hours(3600000));

  // special_values operations
  time_duration pi_dur(pos_infin), ni_dur(neg_infin), ndt_dur(not_a_date_time);
  check("+infin + -infin", pi_dur + ni_dur == ndt_dur);
  check("infin / int", pi_dur / 3 == pi_dur);
  check("infin + duration", pi_dur + td_12 == pi_dur);
  check("infin - duration", pi_dur - td_12 == pi_dur);
  check("unary-", -pi_dur == ni_dur);
  check("-infin less than +infin", ni_dur < pi_dur);
  check("-infin less than duration", ni_dur < td_12);
  check("+infin greater than duration", pi_dur > td_12);
  std::string result(""), answer("+infinity");
  result = to_simple_string(pi_dur);
  check("to string +infin", result==answer);
  result = to_simple_string(ni_dur);
  answer = "-infinity";
  check("to string +infin", result==answer);
  result = to_simple_string(ndt_dur);
  //answer = "not-a-number";
  answer = "not-a-date-time";
  check("to string +infin", result==answer);
 
  using namespace boost::gregorian;
  ptime t1(date(2001,7,14));
  ptime t2(date(2002,7,14));
  check("One year of hours: 365*24=8760",  365*24 == ((t2-t1).hours()));
  check("Total seconds in a year",  365*24*3600 == ((t2-t1).total_seconds()));

  std::cout << to_simple_string(time_duration(20000 * 24, 0, 0, 0)) << std::endl;
  std::cout << to_simple_string(time_duration(24855 * 24, 0, 0, 0)) << std::endl;
  std::cout << to_simple_string(time_duration(24856 * 24, 0, 0, 0)) << std::endl;
  std::cout << to_simple_string(time_duration(25000 * 24, 0, 0, 0)) << std::endl;
  time_duration tdl1(25000*24, 0, 0, 0);
  check("600000 hours", tdl1.hours() == 600000);
  time_duration tdl2(2000000, 0, 0, 0);
  check("2000000 hours", tdl2.hours() == 2000000);

  check("total milliseconds", seconds(1).total_milliseconds() == 1000);
  check("total microseconds", seconds(1).total_microseconds() == 1000000);
  check("total nanoseconds", seconds(1).total_nanoseconds() == 1000000000);
  check("total milliseconds", hours(1).total_milliseconds() == 3600*1000);
  boost::int64_t tms = static_cast<boost::int64_t>(3600)*1000000*1001; //ms per sec
  check("total microseconds 1000 hours", hours(1001).total_microseconds() == tms);
  tms = static_cast<boost::int64_t>(3600)*365*24*1000;
  check("total milliseconds - one year", (t2-t1).total_milliseconds() == tms);
  tms = 3600*365*24*static_cast<boost::int64_t>(1000000000);
  check("total nanoseconds - one year", (t2-t1).total_nanoseconds() == tms);
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
#else
  std::cout << "tms: " << (t2-t1).total_milliseconds() << std::endl;
  std::cout << "nano per year: " << (t2-t1).total_nanoseconds() << std::endl;
#endif
  // make it into a double
  double d1 = microseconds(25).ticks()/(double)time_duration::ticks_per_second();
  std::cout << d1 << std::endl;
  
  //Following causes errors on several compilers about value to large for
  //type.  So it is commented out for now.  Strangely works on gcc 3.3
  //eg: integer constant out of range
//   long sec_in_200k_hours(7200000000); 
//   check("total sec in 2000000 hours", 
//         tdl2.total_seconds() == sec_in_200k_hours);
//   std::cout << to_simple_string(time_duration(2000000, 0, 0, 0)) << std::endl;

  
  return printTestStats();

}


