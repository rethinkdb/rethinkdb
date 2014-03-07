/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include <iostream>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "../testfrmwk.hpp"

void special_values_tests()
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  time_duration td_pi(pos_infin), td_ni(neg_infin), td_ndt(not_a_date_time);
  date_duration dd_pi(pos_infin), dd_ni(neg_infin), dd_ndt(not_a_date_time);
  date d_pi(pos_infin), d_ni(neg_infin), d_ndt(not_a_date_time);
  time_duration td(1,2,3,4);
  date_duration dd(1234);
  date d(2003,Oct,31);

#if !defined(DATE_TIME_NO_DEFAULT_CONSTRUCTOR)
  { // default constructor
    ptime def;
    check("Default constructor", def == ptime(not_a_date_time));
  }
#endif // DATE_TIME_NO_DEFAULT_CONSTRUCTOR

  { // special values construction tests
    ptime p_sv1(pos_infin);
    std::string s("+infinity");
    check("from special value +infinity", to_simple_string(p_sv1) == s);
    ptime result = p_sv1 + dd;
    check("Special value (pos infin) + date_duration = +infinity", to_iso_extended_string(result) == s);
    check("is_special function - pos infin", result.is_special());
    result = p_sv1 - dd;
    check("Special value (pos infin) - date_duration = +infinity", to_iso_extended_string(result) == s);
    result = p_sv1 - dd_ni;
    check("Special value (pos infin) - date_duration (neg infin) = +infinity", to_iso_extended_string(result) == s);
    ptime p_sv2(neg_infin);
    s = "-infinity";
    check("from special value -infinity", to_iso_string(p_sv2) == s);
    result = p_sv2 - td_pi;
    check("Special value (neg infin) - special time_duration (pos infin) = -infinity", to_iso_extended_string(result) == s);
    ptime p_sv3(not_a_date_time);
    check("is_special function - not_a_date_time", p_sv3.is_special());
    s = "not-a-date-time";
    check("from special value NADT", to_iso_extended_string(p_sv3) == s);
    result = p_sv3 + td;
    check("Special value (NADT) + time_duration = NADT", to_iso_extended_string(result) == s);
    result = p_sv3 - td;
    check("Special value (NADT) - time_duration = NADT", to_iso_extended_string(result) == s);
    result = p_sv2 + td_pi;
    check("Special value (neg infin) + special time_duration (pos infin) = NADT", to_iso_extended_string(result) == s);
    result = p_sv1 + dd_ni;
    check("Special value (pos infin) + date_duration (neg infin) = NADT", to_iso_extended_string(result) == s);
    result = p_sv1 + dd_ndt;
    check("Special value (pos infin) - date_duration (NADT) = NADT", to_iso_extended_string(result) == s);
  }
  { // special values construction tests
    ptime p_sv1(d_pi, td);
    std::string s("+infinity");
    check("duration & special_date", to_simple_string(p_sv1) == s);
    ptime p_sv2(d_ni, td);
    s = "-infinity";
    check("duration & special_date", to_iso_string(p_sv2) == s);
    ptime p_sv3(d_ndt, td);
    s = "not-a-date-time";
    check("duration & special_date", to_iso_extended_string(p_sv3) == s);
  }
  { // special values construction tests
    ptime p_sv1(d_ndt, td);
    std::string s("not-a-date-time");
    check("NADT & duration", to_simple_string(p_sv1) == s);
    ptime p_sv2(d, td_ndt);
    check("date & NADT", to_iso_string(p_sv2) == s);
    ptime p_sv3(d_pi, td_ni);
    check("+infinity_date & -infinity_duration", 
    to_iso_extended_string(p_sv3) == s);

  }
  { // special values tests
    ptime p_sv1(d, td_pi), pt(d,td);
    std::string s("+infinity");
    check("special_duration & date", to_simple_string(p_sv1) == s);
    check("ptime::date() +infinity", to_simple_string(p_sv1.date()) == s);
    ptime p_sv2(d, td_ni);
    s = "-infinity";
    check("special_duration & date", to_iso_string(p_sv2) == s);
    check("ptime::time_of_day() -infinity", 
    to_simple_string(p_sv2.time_of_day()) == s);
    ptime p_sv3(d, td_ndt);
    s = "not-a-date-time";
    check("special_duration & date", to_iso_extended_string(p_sv3) == s);
    check("ptime::date() - NADT", to_simple_string(p_sv3.date()) == s);
    check("ptime::time_of_day() - NADT", 
    to_simple_string(p_sv3.time_of_day()) == s);
    check("-infinity less than ...", p_sv2 < p_sv1);
    check("-infinity less than ...", p_sv2 < pt);
    check("+infinity greater than ...", pt < p_sv1);
    check("-infinity less than equal to ...", p_sv2 <= p_sv2);
    check("-infinity less than equal to ...", p_sv2 <= pt);
    check("+infinity greater than equal to ...", p_sv1 >= pt);
    check("not equal", p_sv1 != p_sv2);
    check("not equal", p_sv3 != p_sv2);
    check("not equal", pt != p_sv1);

    check("is_pos_infinity", p_sv1.is_infinity() && p_sv1.is_pos_infinity());
    check("is_neg_infinity", p_sv2.is_infinity() && p_sv2.is_neg_infinity());
    check("is_not_a_date_time", !p_sv3.is_infinity() && p_sv3.is_not_a_date_time());
   
    check("special_ptime + date_duration", p_sv1 + dd == p_sv1);
    check("ptime - special_date_duration", pt - dd_pi == p_sv2);
    check("ptime - special_date_duration", pt - dd_ndt == p_sv3);
    
    check("special_ptime + time_duration", p_sv2 + td == p_sv2);
    check("special_ptime - time_duration", pt - td_ni == p_sv1);
    check("ptime + special_time_duration", pt + td_ndt == p_sv3);
    check("ptime - special_ptime", pt - p_sv1 == td_ni);
    check("ptime - special_ptime", pt - p_sv2 == td_pi);
    check("ptime - special_ptime", pt - p_sv3 == td_ndt);
    check("special_ptime - special_ptime", p_sv2 - p_sv2 == td_ndt);
  }
}

int
main() 
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  date d(2001,Dec,1);
  time_duration td(5,4,3);
  ptime t1(d, td);     //2001-Dec-1 05:04:03
  check("date part check", t1.date() == d);
  check("time part check", t1.time_of_day() == td);
  check("ptime with more than 24 hours", ptime(date(2005,10,30), hours(25)) == ptime(date(2005,10,31),hours(1)));
  ptime t2(t1); //copy constructor
  ptime t3 = t2; //assignment
  check("date part check", t3.date() == d);
  check("time part check", t3.time_of_day() == td);
  check("equality", t1 == t3);
  date d2(2001,Jan,1);
  ptime t4(d2, td);  //2001-Jan-1 05:04:03
  check("equality - not equal", !(t1 == t4));
  time_duration td1(5,4,0);
  ptime t5(d, td1); //2001-Dec-1 05:04:00
  check("equality - not equal", !(t1 == t5));
  check("not equal - not equal", t1 != t5);

  check("less - not less",       !(t1 < t1));
  check("less - less",           t4 < t1);
  check("less - less",           t5 < t1);
  check("less equal - equal",    t1 <= t1);
  check("greater equal - equal", t1 >= t1);

  date_duration twodays(2);
  ptime t6 = t1 + twodays;
  date d3(2001,Dec,3);
  check("operator+(date_duration)", t6 == ptime(d3,td));
  ptime t7 = t1 - twodays;
  check("operator-(date_duration)", t7 == ptime(date(2001,Nov,29),td));
  {
    ptime t6b(date(2003,Oct,31),time_duration(10,0,0,0));
    t6b += date_duration(55);
    check("operator +=(date_duration)", t6b ==
          ptime(date(2003,Dec,25), time_duration(10,0,0,0)));
    t6b += hours(6);
    check("operator +=(time_duration)", t6b ==
          ptime(date(2003,Dec,25), time_duration(16,0,0,0)));
    t6b -= date_duration(55);
    check("operator -=(date_duration)", t6b ==
          ptime(date(2003,Oct,31), time_duration(16,0,0,0)));
    t6b -= hours(6);
    check("operator -=(time_duration)", t6b ==
          ptime(date(2003,Oct,31), time_duration(10,0,0,0)));
    t6b += hours(25);
    check("operator +=(time_duration, more than 24 hours)", t6b ==
          ptime(date(2003,Nov,1), time_duration(11,0,0,0)));
    t6b -= hours(49);
    check("operator -=(time_duration, more than 48 hours)", t6b ==
          ptime(date(2003,Oct,30), time_duration(10,0,0,0)));
  }
  time_duration td2(1,2,3); 
  ptime t8(date(2001,Dec,1)); //midnight
  ptime t9 = t8 + td2; //Dec 2 at 01:02:03
  ptime t10(date(2001,Dec,1),time_duration(1,2,3));
  std::cout << to_simple_string(t9) << std::endl;
  std::cout << to_simple_string(t10) << std::endl;
  std::cout << to_simple_string(td2) << std::endl;
  check("add 2001-Dec-01 0:0:0 + 01:02:03", t9 == t10);
  {
    ptime t9(date(2001,Dec,1), time_duration(12,0,0)); //Dec 1 at Noon
    time_duration td3(-4,0,0);
    check("add 2001-Dec-01 12:00:00 + (-04:00:00)", 
    t9+td3 == ptime(date(2001,Dec,1), time_duration(8,0,0)) );
    std::cout << to_simple_string(t9-td3) << std::endl;
  }
  time_duration td3(24,0,0); // a day
  check("add 2001-Dec-01 0:0:0 + 24:00:00", t8+td3 == ptime(date(2001,Dec,2)));
  time_duration td4(24,0,1); // a day, 1 second
  check("add 2001-Dec-01 0:0:0 + 24:00:01", t8+td4
        == ptime(date(2001,Dec,2), time_duration(0,0,1)));
  //looks like this fails b/c limits are exceeded now that we have subseconds..
  time_duration td5(168,0,1);  //one week 24X7
  check("add 2001-Dec-01 0:0:0 + 168:00:01", t8+td5
        == ptime(date(2001,Dec,8), time_duration(0,0,1)));
  
  //   ptime t10a = t8+td5;
  //   std::cout << to_simple_string(t10a) << std::endl;

  //Subtraction of time duration -- add more!!
  ptime t11(date(2001,Dec,1), time_duration(12,0,0)); //noon
  time_duration td6(12,0,1);
  ptime t12 = t11-td6;
  check("sub 2001-Dec-01 12:0:0 - 12:00:01", 
        t12 == ptime(date(2001,Nov,30), time_duration(23,59,59)));
  
  check("sub 2001-Dec-01 12:0:0 - 13:00:00", 
        (t11-time_duration(13,0,0))== ptime(date(2001,Nov,30), 
                                            time_duration(23,0,0)));
  check("sub 2001-Dec-01 12:0:0 - (-13:00:00)", 
        (t11-time_duration(-13,0,0))== ptime(date(2001,Dec,2), 
                                            time_duration(1,0,0)));
  //  std::cout << to_simple_string(t12.date()) << std::endl;
  
  ptime t13(d, hours(3));
  ptime t14(d, hours(4));
  ptime t14a(d+date_duration(1), hours(4));
  //Subtract 2 times
  std::cout << to_simple_string(t14-t13) << std::endl;
  //  time_duration td7 = 
  check("time subtraction positive result", 
        t14-t13 == hours(1));
  std::cout << to_simple_string(t13-t14) << std::endl;
  check("time subtraction negative result", 
        t13-t14 == hours(-1));
  check("time subtraction positive result", 
        t14a-t14 == hours(24));
  

  ptime t15(d, time_duration(0,0,0,1));
  ptime t16(d, time_duration(0,0,0,2));
  check("time subsecond add test", 
        t15 + time_duration::unit() == t16);
  check("time subsecond sub test", 
        t16 - time_duration::unit() == t15);
  
  ptime t17 = ptime(d) - time_duration::unit();
  std::cout << to_simple_string(t17) << std::endl;
  
  ptime t18(d, hours(25));
  std::cout << to_simple_string(t18) << std::endl;
  
  //time_t conversions:
  t18 = from_time_t(0); //1970-1-1 0:0:0
  check("time_t conversion of 0", t18 == ptime(date(1970,1,1)));
  
  std::time_t tt(500000000); 
  t18 = from_time_t(tt); //1985-11-5 0:53:20
  check("time_t conversion of 500000000", 
        t18 == ptime(date(1985,11,5), time_duration(0,53,20)));
  
  std::time_t tt1(1060483634); 
  t18 = from_time_t(tt1); //2003-08-10 2:47:14
  check("time_t conversion of 1060483634", 
        t18 == ptime(date(2003,8,10), time_duration(2,47,14)));
  
  std::time_t tt2(1760483634); 
  t18 = from_time_t(tt2); //2025-10-14 23:13:54
  check("time_t conversion of 1760483634", 
        t18 == ptime(date(2025,10,14), time_duration(23,13,54)));
  
  std::time_t tt3(1960483634); 
  t18 = from_time_t(tt3); //2032-2-15 18:47:14
  check("time_t conversion of 1960483634", 
        t18 == ptime(date(2032,2,15), time_duration(18,47,14)));

  special_values_tests();

  //min and max constructors
  ptime min_ptime(min_date_time);
  check("check min time constructor", min_ptime == ptime(date(1400,1,1), time_duration(0,0,0,0)));
  //  std::cout << min_ptime << std::endl;
  ptime max_ptime(max_date_time);
  check("check max time constructor", max_ptime == ptime(date(9999,12,31), 
                                                         hours(24)-time_duration::unit())); 
  //  std::cout << max_ptime << std::endl;

  //tm conversion checks
  //converts to date and back -- should get same result -- note no fractional seconds in these times
  check("tm conversion functions 2001-12-1 05:04:03", ptime_from_tm(to_tm(t1)) == t1);
  check("tm conversion functions min date 1400-1-1 ", ptime_from_tm(to_tm(min_ptime)) == min_ptime);
  //this conversion will drop fractional seconds
  check("tm conversion functions max date 9999-12-31 23:59:59.9999 - truncated frac seconds", 
        ptime_from_tm(to_tm(max_ptime)) == ptime(date(max_date_time), time_duration(23,59,59)));
  
  try{
    ptime pt(pos_infin);
    tm pt_tm = to_tm(pt);
    check("Exception not thrown (special_value to_tm)", false);
    //following code does nothing useful but stops compiler from complaining about unused pt_tm
    std::cout << pt_tm.tm_sec << std::endl;
  }catch(std::out_of_range& e){
    check("Caught expected exception (special_value to_tm)", true);
  }catch(...){
    check("Caught un-expected exception (special_value to_tm)", false);
  }
  try{
    // exception is only thrown from gregorian::to_tm. Needed to
    // be sure it always gets thrown.
    ptime pt(date(2002,Oct,31), hours(1));
    pt += time_duration(pos_infin);
    tm pt_tm = to_tm(pt);
    check("Exception not thrown (special_value to_tm)", false);
    //following code does nothing useful but stops compiler from complaining about unused pt_tm
    std::cout << pt_tm.tm_sec << std::endl;
  }catch(std::out_of_range& e){
    check("Caught expected exception (special_value to_tm)", true);
  }catch(...){
    check("Caught un-expected exception (special_value to_tm)", false);
  }

  
  return printTestStats();
  
}

