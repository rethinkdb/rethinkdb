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
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../testfrmwk.hpp"
#include <boost/lexical_cast.hpp>

using namespace boost::gregorian;
using namespace boost::posix_time;
using boost::lexical_cast;

int main(){
#if defined(BOOST_NO_STD_WSTRING) || \
    defined(BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS)
  check("No wstring/wstream support for this compiler", false);
#else
  
  std::wstring res, ws;
  std::wstringstream wss;
  /* time_period was used because all the time-type objects
   * that have operator<< can be easily accessed from it.
   * Tose are: ptime, time_duration, time_period */
  date d1(2003,Jan,20), d2(2003,May,10);
  time_period tp(ptime(d1,hours(1)), ptime(d2,hours(15)));

  // ptime
  wss << tp.begin(); 
  res = L"2003-Jan-20 01:00:00";
  check("ptime op<<", res == wss.str());
  wss.str(L"");
  ws = to_simple_wstring(tp.begin());
  check("ptime to_simple_wstring", res == ws);
  res = L"20030120T010000";
  ws = to_iso_wstring(tp.begin());
  check("ptime to_iso_wstring", res == ws);
  res = L"2003-01-20T01:00:00";
  ws = to_iso_extended_wstring(tp.begin());
  check("ptime to_iso_extended_wstring", res == ws);
  
  // time_duration
  wss << tp.length(); 
  res = L"2654:00:00";
  check("time_duration", res == wss.str());
  wss.str(L"");
  ws = to_simple_wstring(tp.length());
  check("time_duration to_simple_wstring", res == ws);
  res = L"26540000";
  ws = to_iso_wstring(tp.length());
  check("time_duration to_iso_wstring", res == ws);

  // time_period
  wss << tp;
#ifdef BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
  res = L"[2003-Jan-20 01:00:00/2003-May-10 14:59:59.999999999]";
#else
  res = L"[2003-Jan-20 01:00:00/2003-May-10 14:59:59.999999]";
#endif
  check("time_period", res == wss.str());
  wss.str(L"");
  ws = to_simple_wstring(tp);
  check("time_period to_simple_wstring", res == ws);

  // special values
  time_duration sv_td(neg_infin);
  date sv_d(pos_infin);
  ptime sv_tp(sv_d,hours(1));
  res = L"+infinity";
  wss << sv_tp;
  check("ptime op<< special value", res == wss.str());
  wss.str(L"");
  ws = to_simple_wstring(sv_tp);
  check("ptime to_simple_wstring special value", res == ws);
  res = L"-infinity";
  wss << sv_td;
  check("time_duration op<< special value", res == wss.str());
  wss.str(L"");
  ws = to_simple_wstring(sv_td);
  check("time_duration to_simple_wstring special value", res == ws);
  
#endif
  return printTestStats();
}
