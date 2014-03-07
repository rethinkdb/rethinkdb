

/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2009-06-04 01:24:49 -0700 (Thu, 04 Jun 2009) $
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/date_facet.hpp"
#include "../testfrmwk.hpp"
#include <iostream>
#include <sstream>


template<class temporal_type, typename charT>
inline
void
teststreaming(std::string testname,
              temporal_type value,
              std::basic_string<charT> expected_result,
              const std::locale& locale = std::locale::classic())
{
  std::basic_stringstream<charT> ss;
  ss.imbue(locale);
  ss << value;
  check_equal(testname, ss.str(), expected_result);
}


// collections for adding to facet
const char* const month_short_names[]={"*jan*","*feb*","*mar*",
                                       "*apr*","*may*","*jun*",
                                       "*jul*","*aug*","*sep*",
                                       "*oct*","*nov*","*dec*"};

const char* const month_long_names[]={"**January**","**February**","**March**",
                                      "**April**","**May**","**June**",
                                      "**July**","**August**","**September**",
                                      "**October**","**November**","**December**"};

const char* const weekday_short_names[]={"day1", "day2","day3","day4",
                                         "day5","day6","day7"};

const char* const weekday_long_names[]= {"Sun-0", "Mon-1", "Tue-2", 
                                         "Wed-3", "Thu-4", 
                                         "Fri-5", "Sat-6"};

std::vector<std::basic_string<char> > short_weekday_names;
std::vector<std::basic_string<char> > long_weekday_names;
std::vector<std::basic_string<char> > short_month_names;
std::vector<std::basic_string<char> > long_month_names;

#if !defined(BOOST_NO_STD_WSTRING)
// collections of test results
const std::wstring full_months[]={L"January",L"February",L"March",
                                  L"April",L"May",L"June",
                                  L"July",L"August",L"September",
                                  L"October",L"November",L"December"};
const std::wstring short_months[]={L"Jan",L"Feb",L"Mar",
                                   L"Apr",L"May",L"Jun",
                                   L"Jul",L"Aug",L"Sep",
                                   L"Oct",L"Nov",L"Dec"};

const std::wstring full_weekdays[]= {L"Sunday", L"Monday",L"Tuesday", 
                                     L"Wednesday", L"Thursday", 
                                     L"Friday", L"Saturday"};
const std::wstring short_weekdays[]= {L"Sun", L"Mon",L"Tue", 
                                      L"Wed", L"Thu", 
                                      L"Fri", L"Sat"};

//const whcar_t const 
#endif // BOOST_NO_STD_WSTRING

int main() {
  using namespace boost::gregorian;

  std::copy(&month_short_names[0],
            &month_short_names[12],
            std::back_inserter(short_month_names));

  std::copy(&month_long_names[0],
            &month_long_names[12],
            std::back_inserter(long_month_names));

  std::copy(&weekday_short_names[0],
            &weekday_short_names[7],
            std::back_inserter(short_weekday_names));

  std::copy(&weekday_long_names[0],
            &weekday_long_names[7],
            std::back_inserter(long_weekday_names));

  {
    std::stringstream ss;
    date d(2004,Oct,31);
    date_period dp(d, d + days(7));
    ss << d;
    check("to_string & default formats match",
        to_simple_string(d) == ss.str());
    ss.str("");
    ss << dp;
    check("to_string & default formats match",
        to_simple_string(dp) == ss.str());
  }

  {
    date d(2004,Oct, 13);
    date_period dp(d, d + days(7));
    {
      date_facet* datefacet = new date_facet();
      datefacet->format(date_facet::standard_format_specifier);
      std::cout.imbue(std::locale(std::locale::classic(), datefacet));
      teststreaming("default classic date", d, std::string("10/13/04"),
                    std::locale(std::locale::classic(), datefacet));
      std::cout << "default classic date output: " << d << std::endl;

    }
    {
      date_facet* datefacet = new date_facet();
      datefacet->format(date_facet::standard_format_specifier);
      teststreaming("default classic date period", dp, 
                    std::string("[10/13/04/10/19/04]"),
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet();
      datefacet->format("%Y-%d-%b %a");
      teststreaming("custom date facet date period", dp, 
                    std::string("[2004-13-Oct Wed/2004-19-Oct Tue]"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet();
      datefacet->set_iso_format();
      teststreaming("custom date facet date", d, 
                    std::string("20041013"), 
                    std::locale(std::locale::classic(), datefacet));

    }
    {
      date_facet* datefacet = new date_facet();
      datefacet->set_iso_format();
      teststreaming("custom date facet date period", dp, 
                    std::string("[20041013/20041019]"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet();
      datefacet->set_iso_extended_format();
      teststreaming("custom date facet date", d, 
                    std::string("2004-10-13"), 
                    std::locale(std::locale::classic(), datefacet));

    }
    {
      date_facet* datefacet = new date_facet();
      datefacet->set_iso_extended_format();
      teststreaming("custom date facet date period", dp, 
                    std::string("[2004-10-13/2004-10-19]"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet();
      datefacet->set_iso_extended_format();
      period_formatter pf(period_formatter::AS_OPEN_RANGE, " / ", "[ ", " )", " ]");
      datefacet->period_formatter(pf);
      teststreaming("custom date facet date period - open range custom delimeters", dp, 
                    std::string("[ 2004-10-13 / 2004-10-20 )"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet("%A %b %d, %Y");
      datefacet->short_month_names(short_month_names);
      teststreaming("custom date facet -- custom short month names", d, 
                    std::string("Wednesday *oct* 13, 2004"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet("%B %A %d, %Y");
      datefacet->long_month_names(long_month_names);
      teststreaming("custom date facet -- custom long month names", d, 
                    std::string("**October** Wednesday 13, 2004"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet("%a - %b %d, %Y");
      datefacet->short_weekday_names(short_weekday_names);
      std::cout.imbue(std::locale(std::locale::classic(), datefacet));
      std::cout << d << std::endl;
      teststreaming("custom date facet -- custom short weekday names", d, 
                    std::string("day4 - Oct 13, 2004"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {
      date_facet* datefacet = new date_facet("%b %d, %Y ++ %A");
      datefacet->long_weekday_names(long_weekday_names);
      teststreaming("custom date facet -- custom short weekday names", d, 
                    std::string("Oct 13, 2004 ++ Wed-3"), 
                    std::locale(std::locale::classic(), datefacet));
    }

    {//date
      date_facet* datefacet = new date_facet("%Y-%b-%d %%d");
      teststreaming("Literal '%' in date format", d, 
                    std::string("2004-Oct-13 %d"), 
                    std::locale(std::locale::classic(), datefacet));
    }
    {
      date_facet* datefacet = new date_facet("%Y-%b-%d %%%d");
      teststreaming("Multiple literal '%'s in date format", d, 
                    std::string("2004-Oct-13 %13"), 
                    std::locale(std::locale::classic(), datefacet));
    }
    {//month
      date_facet* datefacet = new date_facet();
      datefacet->month_format("%b %%b");
      teststreaming("Literal '%' in month format", d.month(), 
                    std::string("Oct %b"), 
                    std::locale(std::locale::classic(), datefacet));
    }
    {
      date_facet* datefacet = new date_facet();
      datefacet->month_format("%b %%%b");
      teststreaming("Multiple literal '%'s in month format", d.month(), 
                    std::string("Oct %Oct"), 
                    std::locale(std::locale::classic(), datefacet));
    }
    {//weekday
      date_facet* datefacet = new date_facet();
      datefacet->weekday_format("%a %%a");
      teststreaming("Literal '%' in weekday format", d.day_of_week(), 
                    std::string("Wed %a"), 
                    std::locale(std::locale::classic(), datefacet));
    }
    {
      date_facet* datefacet = new date_facet();
      datefacet->weekday_format("%a %%%a");
      teststreaming("Multiple literal '%'s in weekday format", d.day_of_week(), 
                    std::string("Wed %Wed"), 
                    std::locale(std::locale::classic(), datefacet));
    }



    date d_not_date(not_a_date_time);
    teststreaming("special value, no special facet", d_not_date, std::string("not-a-date-time"));


//       std::cout.imbue(std::locale(std::locale::classic(), datefacet));
//       std::cout << d << std::endl;


  }

  // date_generator tests
  {
    partial_date pd(31,Oct);
    teststreaming("partial date", pd, std::string("31 Oct"));
    first_kday_of_month fkd(Tuesday, Sep);
    teststreaming("first kday", fkd, std::string("first Tue of Sep"));
    nth_kday_of_month nkd2(nth_kday_of_month::second, Tuesday, Sep);
    teststreaming("nth kday", nkd2, std::string("second Tue of Sep"));
    nth_kday_of_month nkd3(nth_kday_of_month::third, Tuesday, Sep);
    teststreaming("nth kday", nkd3, std::string("third Tue of Sep"));
    nth_kday_of_month nkd4(nth_kday_of_month::fourth, Tuesday, Sep);
    teststreaming("nth kday", nkd4, std::string("fourth Tue of Sep"));
    nth_kday_of_month nkd5(nth_kday_of_month::fifth, Tuesday, Sep);
    teststreaming("nth kday", nkd5, std::string("fifth Tue of Sep"));
    last_kday_of_month lkd(Tuesday, Sep);
    teststreaming("last kday", lkd, std::string("last Tue of Sep"));
    first_kday_before fkb(Wednesday);
    teststreaming("First before", fkb, std::string("Wed before"));
    first_kday_after fka(Thursday);
    teststreaming("First after", fka, std::string("Thu after"));
  }

#if !defined(BOOST_NO_STD_WSTRING) 
    date d(2004,Oct, 13);
    date_period dp(d, d + days(7));
    date d_not_date(not_a_date_time);
    
    teststreaming("special value, no special facet wide", d_not_date, 
                  std::wstring(L"not-a-date-time"));
  {
    wdate_facet* wdatefacet = new wdate_facet();
    wdatefacet->format(wdate_facet::standard_format_specifier);
    teststreaming("widestream default classic date", d, 
                  std::wstring(L"10/13/04"),
                  std::locale(std::locale::classic(), wdatefacet));
  }
  {
    wdate_facet* wdatefacet = new wdate_facet();
    wdatefacet->format(wdate_facet::standard_format_specifier);
    teststreaming("widestream default classic date period", dp, 
                  std::wstring(L"[10/13/04/10/19/04]"),
                  std::locale(std::locale::classic(), wdatefacet));
  }
  {
    wdate_facet* wdatefacet = new wdate_facet();
    wdatefacet->format(L"%Y-%d-%b %a");
    teststreaming("widestream custom date facet", d, 
                  std::wstring(L"2004-13-Oct Wed"), 
                  std::locale(std::locale::classic(), wdatefacet));
  }
  {
    wdate_facet* wdatefacet = new wdate_facet();
    wdatefacet->format(L"%Y-%d-%b %a");
    teststreaming("widestream custom date facet date period", dp, 
                  std::wstring(L"[2004-13-Oct Wed/2004-19-Oct Tue]"), 
                  std::locale(std::locale::classic(), wdatefacet));
  }
  {
    wdate_facet* wdatefacet = new wdate_facet();
    wdatefacet->set_iso_extended_format();
    wperiod_formatter pf(wperiod_formatter::AS_OPEN_RANGE, L" / ", L"[ ", L" )", L" ]");
    wdatefacet->period_formatter(pf);
    teststreaming("custom date facet date period - open range custom delimeters", dp, 
                  std::wstring(L"[ 2004-10-13 / 2004-10-20 )"), 
                  std::locale(std::locale::classic(), wdatefacet));
  }
  /************* small gregorian types tests *************/ 
  wdate_facet* small_types_facet = new wdate_facet();
  std::locale loc = std::locale(std::locale::classic(), small_types_facet);

  // greg_year test
  greg_year gy(2004);
  teststreaming("greg_year", gy, std::string("2004"));

  // greg_month tests
  {
    for(int i = 0; i < 12; ++i) {
      greg_month m(i+1); // month numbers 1-12
      teststreaming("greg_month short", m, short_months[i], loc);
    }
    small_types_facet->month_format(L"%B"); // full name
    for(int i = 0; i < 12; ++i) {
      greg_month m(i+1); // month numbers 1-12
      teststreaming("greg_month full", m, full_months[i], loc);
    }
  }

  // greg_weekday tests
  {
    for(int i = 0; i < 7; ++i) {
      greg_weekday gw(i); // weekday numbers 0-6
      teststreaming("greg_weekday short", gw, short_weekdays[i], loc);
    }
    small_types_facet->weekday_format(L"%A"); // full name
    for(int i = 0; i < 7; ++i) {
      greg_weekday gw(i); // weekday numbers 0-6
      teststreaming("greg_weekday full", gw, full_weekdays[i], loc);
    }
  }
#endif // BOOST_NO_STD_WSTRING

  return printTestStats();
}

