/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst 
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"
#include "boost/lexical_cast.hpp"
#include <iostream>
#include <string>

// missing or misspelled parts of date string tests
// 'output_str' will be overwritten with what() from caught exception
bool failure_tests(std::string date_spec, 
                   std::string& output_str)
{
  using namespace boost::gregorian;
  bool result = false;
  date d(not_a_date_time);
  try {
    d = from_simple_string(date_spec);
  }
  catch(bad_year& by){ // ex: "205-Jan-15"
    result = true;
    output_str = by.what();
  }
  catch(bad_month& bm){ // ex: "2005-Jsn-15"
    result = true;
    output_str = bm.what();
  }
  catch(bad_day_of_month& bd){ // ex: "2005-Jan-51"
    result = true;
    output_str = bd.what();
  }
  catch(...){
    // test failed - unexpected exception, leave result set to false
  }
  return result;
}

int
main() 
{


// Examples from 8601
// Full date
// Extended CCYY-MM-DD
  std::string s("2001-10-5");

  //This one aborts gcc2.95.3 on mandrake 8.1 linux with
  //bad lexical cast?
  try {
    boost::gregorian::date d(boost::gregorian::from_string(s));
    check("check year",  d.year()  == 2001);
    check("check month", d.month() == 10);
    check("check day",   d.day()   == 5);
  }
  catch(std::exception& e) {
    check("parse 2001-10-5",  false);
    std::cout << "Fail: " << e.what() << std::endl;
  }

  {
    using namespace boost::gregorian;
    // date objects from strings & strings to date objects
    date d(2000, 2, 29);
    date d2 = from_string("2000-2-29");
    check("2000-2-29", d2 == d);
    date d3 = from_string("2000-FEB-29");
    check("2000-FEB-29 (uppercase)", d3 == d);
    date d4 = from_string("2000-february-29");
    check("2000-february-29 (lowercase)", d4 == d);
    date d5 = from_string(to_simple_string(d));
    check("date to string to date", d5 == d);
    date d6 = from_string(to_iso_extended_string(d));
    check("date to string to date", d6 == d);
    date d7 = from_us_string("Feb-29-2000");
    check("date from month-day-year string", d7 == d);
    date d8 = from_uk_string("29-Feb-2000");
    check("date from day-month-year string", d8 == d);
    {
      std::string s("20050229"); // no Feb-29 in 2005
      date d(not_a_date_time);
      try {
        d = date_from_iso_string(s);
        check("Expected exception not thrown: from ISO string (bad_day_of_month)", false);
        std::cout << date_from_iso_string(s) << std::endl;
      }
      catch(bad_day_of_month& e) {
        check("Caught expected exception: bad_day_of_month ", true);
      }
      catch(...) {
        check("Caught unexpected exception", false);
      }
/* not currently passing due to a bug in boost::offset_separator (reported 2005-Aug-02)
      s = "2005022"; // missing a digit
      try {
        d = date_from_iso_string(s);
        check("Expected exception not thrown: from ISO string (missing digit)", false);
        std::cout << date_from_iso_string(s) << std::endl;
      }
      catch(bad_day_of_month& e) {
        check("Caught expected exception: bad_day_of_month ", true);
      }
      catch(...) {
        check("Caught unexpected exception", false);
      }
  */      
      s = "20050228"; // now it's correct
      d = date_from_iso_string(s);
      check("from ISO string", date(2005,Feb,28) == d);
    }
    
    date d9 = from_us_string(__DATE__);
    std::cout << "Today's date: " << to_simple_string(d9) << std::endl;
    date d10 = from_us_string("Feb 29, 2000");
    std::cout << "With comma: " << to_simple_string(d10) << std::endl;
    check("american date with comma: Feb 29, 2000 ", d10 == d);

    date d11 = from_us_string("feb 29 2000");
    check("american date with comma: feb 29 2000 ", d11 == d);

    // test for missing or misspelled date spec components
    std::string output_str("unexpected exception caught");
    check("Year misspelled/out of range: " + output_str,
        failure_tests("205-Jan-15", output_str));
    output_str = "unexpected exception caught";
    check("Month misspelled: " + output_str,
        failure_tests("2005-Jsn-15", output_str));
    output_str = "unexpected exception caught";
    check("Day out of range: " + output_str,
        failure_tests("2005-Jan-55", output_str));
    output_str = "unexpected exception caught";
    check("Missing month and day: " + output_str,
        failure_tests("2005", output_str));
    output_str = "unexpected exception caught";
    check("Missing day: " + output_str,
        failure_tests("2005-Jan", output_str));


#if defined(BOOST_DATE_TIME_NO_LOCALE) || defined(BOOST_NO_STD_ITERATOR_TRAITS) || !defined(USE_DATE_TIME_PRE_1_33_FACET_IO)

    //TODO -- all these PRE_1_33 exclusions need to be removed.  In the meantime, don't make
    //this stuff fail.
#if defined(USE_DATE_TIME_PRE_1_33_FACET_IO)
    check("input streaming for date not available", false); // force a failure
#endif
#else
    {
      std::stringstream ss("2000-2-29");
      ss >> d2;
      check("2000-2-29 stream-in", d2 == d);
    }
    { 
      std::stringstream ss("2000-FEB-29");
      ss >> d2;
      //std::cout << d2 << std::endl;
      check("2000-FEB-29 stream-in (uppercase)", d2 == d);
    }
    {
      std::stringstream ss("2000-february-29");
      ss >> d2;
      check("2000-february-29 stream-in (lowercase)", d2 == d);
    }
    // the removed (3) tests require a stream manipulator for date_order
    // and date_separator (not yet implemented)
    /*{
      std::stringstream ss("Feb-29-2000");
      ss >> d2;
      check("date from month-day-year string stream-in", d2 == d);
    }
    {
      std::stringstream ss("29-Feb-2000");
      ss >> d2;
      check("date from day-month-year string stream-in", d2 == d);
    }
    {
      std::stringstream ss("Feb 29, 2000");
      ss >> d2;
      check("american date with comma: Feb 29, 2000 stream-in", d2 == d);
    }*/
#endif //BOOST_DATE_TIME_NO_LOCALE



    // check proper range
    d = date(2001, 1, 1);
    d2 = from_string("2001-Jan-1");
    d3 = from_string("2001-January-1");
    check("January", d == d2);
    check("January", d == d3);
    d = date(2001, 12, 1);
    d2 = from_string("2001-Dec-1");
    d3 = from_string("2001-December-1");
    check("December", d == d2);
    check("December", d == d3);
#if defined(BOOST_NO_STD_ITERATOR_TRAITS)
    check("date from stream not available: no std iterator traits", false);
#else
    // from stream
    d = date(2000, 10, 31);
    std::stringstream ss("");
    ss << "2000-Oct-31 is Halloween 2k!";
    std::istream_iterator<std::string> iter(ss), eos;
    check("from stream - stringstream", d == from_stream(iter, eos));
#if !(defined(BOOST_NO_STD_WSTRING)) 
#if !(defined(BOOST_DATE_TIME_NO_WISTREAM_ITERATOR))
    std::wstringstream ws;
    ws << "2000-Oct-31 is Halloween 2k!";
    std::istream_iterator<std::wstring, wchar_t> witer(ws), weos;
    check("from stream - wstringstream", d == from_stream(witer, weos));
#endif // NO_WSTREAM_ITERATOR
#endif // BOOST_NO_WSTRING
    char d2_string[] = {"2000-10-31 is Halloween 2k!"};
    char* end = d2_string + sizeof(d2_string) - 1;
    check("from stream - char[]", d == from_stream(d2_string, end));

    std::string s1_string("2000-Oct-31 is Halloween 2k!");
    std::string::iterator s1_start = s1_string.begin();
    std::string::iterator s1_end = s1_string.end();
    check("from stream - string", d == from_stream(s1_start, s1_end));
#ifndef BOOST_NO_STD_WSTRING
    std::wstring w1_string(boost::lexical_cast<std::wstring>("2000-Oct-31 is Halloween 2k!"));
    std::wstring::iterator w1_start = w1_string.begin();
    std::wstring::iterator w1_end = w1_string.end();
    check("from stream - wstring", d == from_stream(w1_start, w1_end));
#endif // BOOST_NO_STD_WSTRING 
#endif // BOOST_NO_STD_ITERATOR_TRAITS
    /* date objects from strings & strings to date objects
     * with misspelled months */
    try {
      date bd = from_string("2002-Jull-4");
      std::cout << "Shouldn't be reached." << 
        boost::gregorian::to_simple_string(bd) << std::endl;
    }
    catch(boost::gregorian::bad_month&){
      check("bad spelling 'Jull'", true);
    }
    catch(std::exception& e){
      check("bad spelling", false);
      std::cout << "Fail: " << e.what() << std::endl;
    }
  }

  
  try {
    std::string s2("2001-12-41"); //oops should be 31
    boost::gregorian::date bad_day(boost::gregorian::from_string(s2)); //won't construct
    check("check bad day", false);
    //The line below won't execute, but make the compiler think
    //we are using bad day....
    std::cout << "Oh oh, this shouldn't be reached: "
              << boost::gregorian::to_iso_string(bad_day) << std::endl;

  }
  catch(boost::gregorian::bad_day_of_month&) { //expected
    check("check bad day", true);
  }
  catch(std::exception& e) {
    //oops wrong exception
    check("check bad day",  false);
    std::cout << "Fail: " << e.what() << std::endl;
  }

  try {
    std::string s2("2001-02-29"); //oops should be 28
    boost::gregorian::date bad_day(boost::gregorian::from_string(s2)); //won't construct
    check("check bad leap year", false);
    //The line below won't execute, but make the compiler think
    //we are using bad day....
    std::cout << "Oh oh, this shouldn't be reached: "
              << boost::gregorian::to_iso_string(bad_day) << std::endl;

  }
  catch(boost::gregorian::bad_day_of_month&) { //expected
    check("check bad leap year", true);
  }
  catch(std::exception& e) {
    //oops wrong exception
    check("check bad leap year",  false);
    std::cout << "Fail: " << e.what() << std::endl;
  }

  try {
    std::string s2("2001-14-1"); //oops should be <= 12
    boost::gregorian::date bad_month(boost::date_time::parse_date<boost::gregorian::date>(s2));
    check("check bad month", false); //fail the test
    //The line below won't execute, but make the compiler think
    //we are using bad day....
    std::cout << "Oh oh, this shouldn't be reached: "
              << boost::gregorian::to_iso_string(bad_month) << std::endl;

  }
  catch(boost::gregorian::bad_month&) { //expected
    check("check bad month", true);
  }
  catch(std::exception& e) {
    //oops wrong exception
    check("check bad month",  false);
    std::cout << "Fail: " << e.what() << std::endl;
  }

  //This one aborts gcc2.95.3 on mandrake 8.1 linux with
  //bad lexical cast?
  try {
    //Example of ISO Standard -- CCYYMMDD
    using namespace boost::gregorian;
    std::string ud("20011009"); //2001-Oct-09
    date d1(boost::gregorian::from_undelimited_string(ud));
    //  std::cout << to_string(d1) << std::endl;
    check("undelimited date string", d1 == date(2001,Oct,9));
    
  
    std::string ad("2001/10/09");
    date d2(boost::date_time::parse_date<date>(ad));
    check("check american date",  d2  == date(2001,Oct,9));
  }
  catch(std::exception& e) {
    check("more parsing",  false);
    std::cout << "Fail: " << e.what() << std::endl;
  }

  using namespace boost::gregorian;
  std::string s2("2003-07-28");
  date d2(from_string(s2));
  check("check date", d2.month() == 7 &&
                      d2.year() == 2003 &&
                      d2.day() == 28);
//   std::string s1("2001-Oct-5");
//   gregorian::date d1(parse_date<gregorian::date>(s1));
//    check("check month", d1.month() == 10);
  

  //Check that the from_string and to_string can be reversed
  date d10(2003, 10, 19);
  std::string d10s = to_simple_string(d10);
  date d11 = from_simple_string(d10s);
  check("to from string inversion", d10 == d11);

  try {
    using namespace boost::gregorian;
    std::string ud(""); //empty string error sf bug# 1155556
    date d1(from_simple_string(ud));
    check("empty string",  false); //should never reach this piont
  }
  catch(std::exception& e) {
    check(std::string("empty string parse (exception expected): ") + e.what(),  true);
  }
  

//Calendar Week + Day Number
// CCYYWwwDThhmmss
// 1986W105T
// week == 10 day=5
// see page 5


//Duration rep
//CCYYMMDDThhmmss/PnYnMnDTnHnMnS

  return printTestStats();

}

