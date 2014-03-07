/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"

#ifndef BOOST_DATE_TIME_NO_LOCALE

const char* const de_short_month_names[]={"Jan","Feb","Mar","Apr","Mai","Jun","Jul","Aug","Sep","Okt","Nov","Dez", "NAM"};

const char* const de_long_month_names[]={"Januar","Februar","Marz","April","Mai","Juni","Juli","August","September","Oktober","November","Dezember","NichtDerMonat"};
const char* const de_special_value_names[]={"NichtDatumzeit","-unbegrenztheit", "+unbegrenztheit"};

const char* const de_short_weekday_names[]={"Son", "Mon", "Die","Mit", "Don", "Fre", "Sam"};

const char* const de_long_weekday_names[]={"Sonntag", "Montag", "Dienstag","Mittwoch", "Donnerstag", "Freitag", "Samstag"};

#endif

int
main() 
{
#ifndef BOOST_DATE_TIME_NO_LOCALE
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  std::stringstream ss;
  date d1(2002,May,1);
  ptime t1(d1, hours(12)+minutes(10)+seconds(5));
  time_duration td0(12,10,5,123);
  ptime t0(d1, td0);
  ss << t0;
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  check("check time output: "+ss.str(), 
        ss.str() == std::string("2002-May-01 12:10:05.000000123"));
#else
  check("check time output: "+ss.str(), 
        ss.str() == std::string("2002-May-01 12:10:05.000123"));
#endif // BOOST_DATE_TIME_HAS_NANOSECONDS
  //  ss.imbue(global); 
  time_period tp(t1, ptime(d1, hours(23)+time_duration::unit()));
  ss.str("");
  ss << tp;
  check("check time period output: "+ ss.str(), 
        ss.str() == std::string("[2002-May-01 12:10:05/2002-May-01 23:00:00]"));

  //Send out the same time with german
  typedef boost::date_time::all_date_names_put<greg_facet_config> date_facet;
 
  ss.imbue(std::locale(std::locale::classic(),
                       new date_facet(de_short_month_names, 
                                      de_long_month_names,
                                      de_special_value_names,
                                      de_short_weekday_names,
                                      de_long_weekday_names,
                                      '.',
                                      boost::date_time::ymd_order_dmy,
                                      boost::date_time::month_as_short_string))); 
  ss.str("");
  ss << t1;
  check("check time output: "+ ss.str(), 
        ss.str() == std::string("01.Mai.2002 12:10:05"));


  time_duration td(5,4,3);
  time_duration td1(-1,25,0), td2(0,-40,0);
  ss.str("");
  ss << td;
  check("check time period output: "+ ss.str(), 
        ss.str() == std::string("05:04:03"));
  ss.str("");
  ss << td1;
  check("check time period output: "+ ss.str(), 
        ss.str() == std::string("-01:25:00"));
  ss.str("");
  ss << td2;
  check("check time period output: "+ ss.str(), 
        ss.str() == std::string("-00:40:00"));


  ss.str("");
  ss << tp;
  check("check time period output - german: "+ ss.str(), 
        ss.str() == std::string("[01.Mai.2002 12:10:05/01.Mai.2002 23:00:00]"));

  /* Input streaming is only available for compilers that  
   * do not have BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS defined */
#ifndef BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS
  
  /****** test streaming in for time classes ******/
  {
    std::istringstream iss("01:02:03.000004 garbage");
    iss >> td;
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
    check("Stream in time_duration", td == time_duration(1,2,3,4000));
#else
    check("Stream in time_duration", td == time_duration(1,2,3,4));
#endif
  }
#if !defined(BOOST_NO_STD_ITERATOR_TRAITS) // vc6 & vc7
  {
    std::istringstream iss("2003-May-13 01:02:03");
    iss >> t1;
    check("Stream in ptime", t1 == ptime(date(2003,May,13), time_duration(1,2,3)));
    std::istringstream iss2("2003-January-13 01:02:03");
    iss2 >> t1;
    check("Stream in ptime2", t1 == ptime(date(2003,Jan,13), time_duration(1,2,3)));
    std::istringstream iss3("2003-feb-13 11:10:09");
    iss3 >> t1;
    check("Stream in ptime3", t1 == ptime(date(2003,Feb,13), time_duration(11,10,9)));

    try {
      std::istringstream iss4("2003-xxx-13 11:10:09");
      iss3 >> t1;
      check("Stream bad ptime", false); //never reach here, bad month exception
    }
    catch(std::exception& e) {
      std::cout << "Got expected exception: " << e.what() << std::endl;
      check("Stream bad ptime", true); 
    }

  }
  {
    date d1(2001,Aug,1), d2(2003,May,13);
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
    time_duration td1(15,32,18,20304000), td2(1,2,3);
#else
    time_duration td1(15,32,18,20304), td2(1,2,3);
#endif

    time_period result(ptime(d1,td1), ptime(d2,td2));
    std::istringstream iss("[2001-Aug-01 15:32:18.020304/2003-May-13 01:02:03]");
    iss >> tp;
    check("Stream in time_period", tp == result);
  }

#if !defined(BOOST_NO_STD_WSTRING)
  /*** wide streaming ***/
  {
    std::wistringstream wiss1(L"01:02:03");//.000004");
    wiss1 >> td;
    check("Wide stream in time_duration", td == time_duration(1,2,3));
    
    std::wistringstream wiss2(L"2003-May-23 03:20:10");
    wiss2 >> t1;
    check("Wide stream in ptime", t1 == ptime(date(2003,May,23), time_duration(3,20,10)));
    
    std::wistringstream wiss3(L"[2004-Jan-01 02:03:04/2004-May-13 01:00:00]");
    wiss3 >> tp;
    date d1 = date(2004,Jan,1);
    date d2 = date(2004,May,13);
    time_duration td1 = time_duration(2,3,4);
    time_duration td2 = time_duration(1,0,0);
    time_period result = time_period(ptime(d1,td1), ptime(d2,td2));
    check("Wide stream in time_period", tp == result);
  }
#else
  check("Wide streaming not available for this compiler", false);
#endif // BOOST_NO_STD_WSTRING

#else  // BOOST_NO_STD_ITERATOR_TRAITS
  check("Streaming in of alphabetic dates (Ex: 2003-Aug-21) \
      not supported by this compiler", false);
#endif // BOOST_NO_STD_ITERATOR_TRAITS

#else // BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS
  check("Streaming in of time classes not supported by this compiler", false);
#endif // BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS
  
#else  //BOOST_DATE_TIME_NO_LOCALE
  check("No tests executed - Locales not supported by this compiler", false);
#endif //BOOST_DATE_TIME_NO_LOCALE

  return printTestStats();
}

