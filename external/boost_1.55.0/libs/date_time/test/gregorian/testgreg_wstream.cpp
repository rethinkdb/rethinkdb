/* Copyright (c) 2003-2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */
#include <iostream>
#include <sstream>
#include <boost/date_time/gregorian/gregorian.hpp>
#include "../testfrmwk.hpp"

using namespace boost::gregorian;

int main(){
#if defined(BOOST_NO_STD_WSTRING) || \
    defined(BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS)
  check("No wstring/wstream support for this compiler", false);
#else
  std::wstring res, ws;
  std::wstringstream wss;
  /* date_period is used because almost all the date-type objects 
   * that have the operator<< can be easily accessed from it.
   * Those are: date, greg_month, greg_day_of_week,
   * date_period, date_duration (also date_generators) */
  date_period dp(date(2003,Aug,21), date(2004,May,27));
 
  
  // date
  wss << dp.begin();
  res = L"2003-Aug-21";
  check("date operator<<", wss.str() == res);
  wss.str(L"");
  ws = to_simple_wstring(dp.begin());
  check("date to_simple_string", ws == res);
  ws = to_iso_wstring(dp.begin());
  res = L"20030821";
  check("date to_iso_string", ws == res);
  ws = to_iso_extended_wstring(dp.begin());
  res = L"2003-08-21";
  check("date to_iso_extended_string", ws == res);
  ws = to_sql_wstring(dp.begin());
  check("date to_sql_string", ws == res);
  wss.str(L"");
#ifndef BOOST_NO_STD_ITERATOR_TRAITS
  { 
    res = L"2003-Aug-21";
    std::wstringstream wss(L"2003-Aug-21");
    date testdate(not_a_date_time);
    wss >> testdate;
    check("date operator>>", to_simple_wstring(testdate) == res);
  }
#else
  check("no date operator>> for this compiler", false);
#endif
  
  // greg_month
  wss << dp.begin().month();
  res = L"08";
  check("greg_month", wss.str() == res);
  wss.str(L"");
  ws = dp.begin().month().as_short_wstring();
  res = L"Aug";
  check("greg_month as_short_wstring", ws == res);
  ws = dp.begin().month().as_long_wstring();
  res = L"August";
  check("greg_month as_long_wstring", ws == res);
  /*{ 
    std::wstringstream wss(L"August");
    greg_month testmonth(not_a_date_time);
    wss >> testmonth;
    check("greg_month operator>>", to_simple_wstring(testmonth) == res);
  }*/
  
  // greg_day_of_week
  wss << dp.begin().day_of_week();
  res = L"Thu";
  check("greg_day_of_week", wss.str() == res);
  wss.str(L"");
  ws = dp.begin().day_of_week().as_short_wstring();
  check("greg_day_of_week as_short_wstring", ws == res);
  ws = dp.begin().day_of_week().as_long_wstring();
  res = L"Thursday";
  check("greg_day_of_week as_long_wstring", ws == res);
  /*{ 
    std::wstringstream wss(L"Thu");
    greg_day_of_week testday(not_a_date_time);
    wss >> testday;
    check("greg_day_of_week operator>>", to_simple_wstring(testday) == res);
  }*/
  
  // date_period
  wss << dp;
  res = L"[2003-Aug-21/2004-May-26]";
  check("date_period", wss.str() == res);
  wss.str(L"");
  ws = to_simple_wstring(dp);
  check("date_period to_simple_string", ws == res);
  res = L"20030821/20040526";
  ws = to_iso_wstring(dp);
  check("date_period to_iso_string", ws == res);
  { 
    std::wstringstream wss(L"[2003-Aug-21/2004-May-27]");
    res = L"[2003-Aug-21/2004-May-26]";
    // following line gives an ambiguous overload of op>>
    //date_period testperiod(date(not_a_date_time),date_duration(not_a_date_time));
    date_period testperiod(date(2003,Aug,21), date(2004,May,27));
    wss >> testperiod;
    check("date_period operator>>", to_simple_wstring(testperiod) == res);
  }
  
  // date_duration
  wss << dp.length();
  res = L"280";
  check("date_duration", wss.str() == res);
  wss.str(L"");
  { 
    std::wstringstream wss(L"280");
    date_duration testduration(not_a_date_time);
    wss >> testduration;
    check("date_duration operator>>", testduration.days() == 280);
  }

  // special values
  date sv_d(neg_infin);
  date_duration sv_dd(pos_infin);
  //date_period sv_dp(sv_d,sv_dd);
  // sv-date
  wss << sv_d;
  res = L"-infinity";
  check("date operator<< special value", wss.str() == res);
  wss.str(L"");
  ws = to_simple_wstring(sv_d);
  check("date to_simple_string special value", ws == res);
  // sv-date_duration
  wss << sv_dd;
  res = L"+infinity";
  check("date_duration operator<< special value", wss.str() == res);
  wss.str(L"");
  // sv-date_period
  /*
   * works - just gives unexpected results:
   *[-infinity/not-a-date-time]
  wss << sv_dp;
  res = L"[2003-Aug-21/2004-May-26]";
  check("date_period", wss.str() == res);
  std::wcout << wss.str() << std::endl;
  wss.str(L"");
  */

  // date_generators
  first_day_of_the_week_after fka(Monday);
  wss << fka;
  res = L"Mon after";
  check("first_kday_after", wss.str() == res);
  wss.str(L"");
  
  first_day_of_the_week_before fkb(Monday);
  wss << fkb;
  res = L"Mon before";
  check("first_kday_before", wss.str() == res);
  wss.str(L"");
  
  first_day_of_the_week_in_month fkom(Monday,Jan);
  wss << fkom;
  res = L"first Mon of Jan";
  check("first_kday_of_month", wss.str() == res);
  wss.str(L"");
  
  last_day_of_the_week_in_month lkom(Monday,Jan);
  wss << lkom;
  res = L"last Mon of Jan";
  check("last_kday_of_month", wss.str() == res);
  wss.str(L"");

#endif
  return printTestStats();
}
