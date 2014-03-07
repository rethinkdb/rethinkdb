
/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland
 */


#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"
#include "boost/date_time/format_date_parser.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

const wchar_t* const wmonth_short_names[]={L"Jan",L"Feb",L"Mar",
                                          L"Apr",L"May",L"Jun",
                                          L"Jul",L"Aug",L"Sep",
                                          L"Oct",L"Nov",L"Dec"};

const char* const month_short_names[]={"Jan","Feb","Mar",
                                       "Apr","May","Jun",
                                       "Jul","Aug","Sep",
                                       "Oct","Nov","Dec"};

const char* const month_long_names[]={"January","February","March",
                                      "April","May","June",
                                      "July","August","September",
                                      "October","November","December"};

const wchar_t* const wmonth_long_names[]={L"January",L"February",L"March",
                                          L"April",L"May",L"June",
                                          L"July",L"August",L"September",
                                          L"October",L"Novomber",L"December"};

const wchar_t* const wweek_short_names[]= {L"Sun", L"Mon", L"Tue", L"Wed",
                                           L"Thu", L"Fri", L"Sat"};

const char* const week_short_names[]={"Sun", "Mon","Tue","Wed",
                                      "Thu","Fri","Sat"};

const wchar_t* const wweek_long_names[]= {L"Sunday", L"Monday", L"Tuesday", 
                                          L"Wednesday", L"Thursday", 
                                          L"Friday", L"Saturday"};

const char* const week_long_names[]= {"Sunday", "Monday", "Tuesday", 
                                      "Wednesday", "Thursday", 
                                      "Friday", "Saturday"};




std::vector<std::basic_string<char> > short_month_names;
std::vector<std::basic_string<wchar_t> > wshort_month_names;
std::vector<std::basic_string<char> > long_month_names;
std::vector<std::basic_string<wchar_t> > wlong_month_names;
std::vector<std::basic_string<char> > short_week_names;
std::vector<std::basic_string<wchar_t> > wshort_week_names;
std::vector<std::basic_string<char> > long_week_names;
std::vector<std::basic_string<wchar_t> > wlong_week_names;


using namespace boost::gregorian;

void
wtest_format(const std::basic_string<wchar_t>& format,
             const std::basic_string<wchar_t>& value,
             const std::string& testname,
             boost::gregorian::date expected_res)
{
  typedef boost::date_time::format_date_parser<date, wchar_t> parser_type;
  typedef std::basic_string<wchar_t> string_type;
  typedef std::istreambuf_iterator<wchar_t> iter_type;
  try {
    //    string_type format(format);
    std::basic_stringstream<wchar_t> ws;
    ws << value;
    iter_type sitr(ws);
    iter_type stream_end;

    parser_type p(format, wshort_month_names, wlong_month_names,
                  wshort_week_names, wlong_week_names);
    date d = p.parse_date(sitr, stream_end, format);
    check_equal(testname, d, expected_res);
  }
  catch(std::exception& e) {
    std::cout << "Got an exception: " << e.what() << std::endl;
    check(testname, false);
  }
}


void
test_format(const std::basic_string<char>& format,
            const std::basic_string<char>& value,
            const std::string& testname,
            boost::gregorian::date expected_res)
{
  typedef boost::date_time::format_date_parser<date, char> parser_type;
  typedef std::basic_string<char> string_type;
  typedef std::istreambuf_iterator<char> iter_type;
  try {
    string_type format(format);
    std::basic_stringstream<char> ws;
    ws << value; 
    iter_type sitr(ws);
    iter_type stream_end;

    parser_type pt(format, short_month_names, long_month_names,
                   short_week_names, long_week_names);
    date d = pt.parse_date(sitr, stream_end, format);
    check_equal(testname, d, expected_res);
  }
  catch(std::exception& e) {
    std::cout << "Got an exception: " << e.what() << std::endl;
    check(testname, false);
  }
}


template<typename charT>
void
test_format2(boost::date_time::format_date_parser<date, charT>& parser,
             const charT* const format,
             const charT* const value,
             const std::string& testname,
             boost::gregorian::date expected_res)
{
  try {
    date d = parser.parse_date(value, format);
    check_equal(testname, d == expected_res);
  }
  catch(std::exception& e) {
    std::cout << "Got an exception: " << e.what() << std::endl;
    check(testname, false);
  }
}

int
main()
{
  std::copy(&wmonth_short_names[0], 
            &wmonth_short_names[12],
            std::back_inserter(wshort_month_names));

  std::copy(&month_short_names[0], 
            &month_short_names[12],
            std::back_inserter(short_month_names));

  std::copy(&month_long_names[0], 
            &month_long_names[12],
            std::back_inserter(long_month_names));

  std::copy(&wmonth_long_names[0], 
            &wmonth_long_names[12],
            std::back_inserter(wlong_month_names));


  std::copy(&wweek_short_names[0], 
            &wweek_short_names[7],
            std::back_inserter(wshort_week_names));

  std::copy(&week_short_names[0], 
            &week_short_names[7],
            std::back_inserter(short_week_names));

  std::copy(&wweek_long_names[0], 
            &wweek_long_names[7],
            std::back_inserter(wlong_week_names));

  std::copy(&week_long_names[0], 
            &week_long_names[7],
            std::back_inserter(long_week_names));

  
  wtest_format(L"1%%23%Y %m %d", L"1232004 12 31 other stuff...",
               "wide and weird", date(2004,12,31));

  wtest_format(L"%Y-%m-%d", L"2004-12-31",
               "%Y-%m-%d wide", date(2004,12,31));

  wtest_format(L"%Y day %j", L"2004 day 001",
               "%Y day %j wide", date(2004,1,1));

  test_format("%m/%d/%y", "10/31/04",
              "%m/%d/%y", date(2004,10,31));


  wtest_format(L"%Y/%m/%d", L"2004-12-31",
               "%Y/%m/%d wide 2004-12-31 input", date(2004,12,31));

  test_format("%Y.%d.%m", "2004.31.1",
              "%Y.%d.%m var length", date(2004,Jan,31));

  test_format("%d.%m.%Y", "1.1.2004",
              "%d.%m.%Y var length month and day", date(2004,1,1));

  test_format("%Y.%m.%d", "2004.1.31",
              "%Y.%m.%d var length month", date(2004,Jan,31));

  test_format("%Y.%b.%d", "2004.Jan.1",
              "%Y.%b.%d var length month", date(2004,Jan,1));

  test_format("%Y%m%d", "20041231",
              "%Y%m%d undelimited", date(2004,12,31));

  test_format("%Y/%d/%b", "2004/01/Jan",
              "%Y/%d/%b month at end", date(2004,1,1));

  test_format("%Y/%b/%d", "2004/Jan/01",
              "%Y/%b/%d named month jan", date(2004,1,1));

  test_format("%Y/%b/%d", "2004/Dec/20",
              "%Y/%b/%d named month dec", date(2004,12,20));

  wtest_format(L"%Y/%b/%d", L"2004-Jul-31",
               "%Y/%b/%d wide 2004-Jul-31 input", date(2004,7,31));

  wtest_format(L"%B %d, %Y", L"March 15, 2004",
               "%B %d, %Y", date(2004,3,15));

  wtest_format(L"%a %B %d, %Y", L"Sun March 15, 2004",
               "%a %B %d, %Y", date(2004,3,15));

  wtest_format(L"%A %B %d, %Y", L"Sunday March 15, 2004",
               "%A %B %d, %Y", date(2004,3,15));

  // bad format case...

  {
    try {
      std::wstring format(L"%Y-%d");
      std::wstringstream ws;
      ws << L"2004-12-31";
      std::istreambuf_iterator<wchar_t> sitr(ws);
      std::istreambuf_iterator<wchar_t> stream_end;

      boost::date_time::format_date_parser<date,wchar_t> pt(format, 
                                                            wshort_month_names,
                                                            wlong_month_names,
                                                            wshort_week_names,
                                                            wlong_week_names);
      date d = pt.parse_date(sitr, stream_end);
      check("Bad format spec test", false);
    }
    catch(std::exception& e) {
      std::cout << "Got an expected exception: " << e.what() << std::endl;
      check("Bad format spec test -- pass", true);

    }
  }

  {
    //some interesting month names 
    const char* const roman_months[]={"I","II","III",
                                      "IV","V","VI",
                                      "VII","VIII","IX",
                                      "X","XI","XII"};
    std::vector<std::basic_string<char> > roman_month_names;
    std::copy(&roman_months[0], 
              &roman_months[12],
              std::back_inserter(roman_month_names));

    std::string format("%Y.%b.%d");

    boost::date_time::format_date_parser<date,char> parser(format, 
                                                           roman_month_names,
                                                           long_month_names,
                                                           short_week_names,
                                                           long_week_names);


    test_format2(parser, "%Y.%b.%d", "2004-I-1",
                 "roman I", date(2004,Jan,1));
    test_format2(parser, "%Y.%b.%d", "2004-II-01",
                 "roman II", date(2004,Feb,1));
    test_format2(parser, "%Y.%b.%d", "2004-III-01",
                 "roman III", date(2004,Mar,1));
    test_format2(parser, "%Y.%b.%d", "2004-IV-01",
                 "roman IV", date(2004,Apr,1));
    test_format2(parser, "%Y.%b.%d", "2004-V-01",
                 "roman V", date(2004,May,1));
    test_format2(parser, "%Y.%b.%d", "2004-VI-01",
                 "roman VI", date(2004,Jun,1));
    test_format2(parser, "%Y.%b.%d", "2004-VII-01",
                 "roman VII", date(2004,Jul,1));
    test_format2(parser, "%Y.%b.%d", "2004-VIII-01",
                 "roman VIII", date(2004,Aug,1));
    test_format2(parser, "%Y.%b.%d", "2004-IX-01",
                 "roman IX", date(2004,Sep,1));
    test_format2(parser, "%Y.%b.%d", "2004-X-01",
                 "roman X", date(2004,Oct,1));
    test_format2(parser, "%Y.%b.%d", "2004-XI-01",
                 "roman XI", date(2004,Nov,1));
    test_format2(parser, "%Y.%b.%d", "2004 XII 1",
                 "roman XII", date(2004,Dec,1));

  }

  { 
    //alternate constructor that takes month/weekday strings
    //from a locale
    std::string format("%Y %m %d");
    boost::date_time::format_date_parser<date,char> parser(format, 
                                                           std::locale::classic());
    test_format2(parser, "%a %Y.%b.%d", "Sun 2004 Jan 1",
                 "strings from locale", date(2004,Jan,1));
    test_format2(parser, "%a %Y.%b.%d", "Mon 2004 Feb 1",
                 "strings from locale", date(2004,Feb,1));
    test_format2(parser, "%a %Y.%b.%d", "Tue 2004 Mar 1",
                 "strings from locale", date(2004,Mar,1));
    test_format2(parser, "%a %Y.%b.%d", "Wed 2004 Apr 1",
                 "strings from locale", date(2004,Apr,1));
    test_format2(parser, "%a %Y.%b.%d", "thu 2004 May 1",
                 "strings from locale", date(2004,May,1));
    test_format2(parser, "%a %Y.%b.%d", "fri 2004 Jun 1",
                 "strings from locale", date(2004,Jun,1));
    test_format2(parser, "%a %Y.%b.%d", "sat 2004 Jul 1",
                 "strings from locale", date(2004,Jul,1));
    test_format2(parser, "%Y.%b.%d", "2004 Aug 1",
                 "strings from locale", date(2004,Aug,1));
    test_format2(parser, "%Y.%b.%d", "2004 Sep 1",
                 "strings from locale", date(2004,Sep,1));
    test_format2(parser, "%Y.%b.%d", "2004 Sep 1",
                 "strings from locale", date(2004,Sep,1));
    test_format2(parser, "%Y.%b.%d", "2004 Oct 1",
                 "strings from locale", date(2004,Oct,1));
    test_format2(parser, "%Y.%b.%d", "2004 Nov 1",
                 "strings from locale", date(2004,Nov,1));
    test_format2(parser, "%Y.%b.%d", "2004 Dec 1",
                 "strings from locale", date(2004,Dec,1));

    test_format2(parser, "%A %B %d, %Y", "Sunday January 1, 2004",
                 "long strings from locale", date(2004,Jan,1));
    test_format2(parser, "%A %B %d, %Y", "Monday February 29, 2004",
                 "long strings from locale", date(2004,Feb,29));
    test_format2(parser, "%A %B %d, %Y", "Tuesday March 1, 2004",
                 "long strings from locale", date(2004,Mar,1));
    test_format2(parser, "%A %B %d, %Y", "Wednesday APRIL 1, 2004",
                 "long strings from locale", date(2004,Apr,1));
    test_format2(parser, "%A %B %d, %Y", "thursday may 15, 2004",
                 "long strings from locale", date(2004,May,15));
    test_format2(parser, "%A %B %d, %Y", "friday june 15, 2004",
                 "long strings from locale", date(2004,Jun,15));
    test_format2(parser, "%A %B %d, %Y", "saturday july 30, 2004",
                 "long strings from locale", date(2004,Jul,30));
    test_format2(parser, "%A %B %d, %Y", "thursday auguST 15, 2004",
                 "long strings from locale", date(2004,Aug,15));
    test_format2(parser, "%A %B %d, %Y", "thursday september 1, 2004",
                 "long strings from locale", date(2004,Sep,1));
    test_format2(parser, "%A %B %d, %Y", "thursday october 1, 2004",
                 "long strings from locale", date(2004,Oct,1));
    test_format2(parser, "%A %B %d, %Y", "thursday november 1, 2004",
                 "long strings from locale", date(2004,Nov,1));
    test_format2(parser, "%A %B %d, %Y", "thursday december 31, 2004",
                 "long strings from locale", date(2004,Dec,31));

  }

  return printTestStats();

}
