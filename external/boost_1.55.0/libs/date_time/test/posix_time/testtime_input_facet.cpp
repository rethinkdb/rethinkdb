/* Copyright (c) 2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2012-09-22 09:04:10 -0700 (Sat, 22 Sep 2012) $
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "../testfrmwk.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// for tests that are expected to fail and throw exceptions
template<class temporal_type, class exception_type>
bool failure_test(temporal_type component,
                  const std::string& input,
                  exception_type const& /*except*/,
                  boost::posix_time::time_input_facet* facet)
{
  using namespace boost::posix_time;
  bool result = false;
  std::istringstream iss(input);
  iss.exceptions(std::ios_base::failbit); // turn on exceptions
  iss.imbue(std::locale(std::locale::classic(), facet));
  try {
    iss >> component;
  }
  catch(exception_type& e) {
    std::cout << "Expected exception caught: \"" 
              << e.what() << "\"" << std::endl;
    result = iss.fail(); // failbit must be set to pass test
  }
  catch(...) {
    result = false;
  }

  return result;
}

// for tests that are expected to fail quietly
template<class temporal_type>
bool failure_test(temporal_type component,
                  const std::string& input,
                  boost::posix_time::time_input_facet* facet)
{
  using namespace boost::posix_time;
  std::istringstream iss(input);
  /* leave exceptions turned off
   * iss.exceptions(std::ios_base::failbit); */
  iss.imbue(std::locale(std::locale::classic(), facet));
  try {
    iss >> component;
  }
  catch(...) {
    std::cout << "Caught unexpected exception" << std::endl;
    return false;
  }

  return iss.fail(); // failbit must be set to pass test
}

using namespace boost::gregorian;
using namespace boost::posix_time;
 

void
do_all_tests()
{

#if defined(USE_DATE_TIME_PRE_1_33_FACET_IO) // skip this file
  check("No tests run for this compiler. Incompatible IO", true);
#else

  // set up initial objects
  time_duration td = hours(0);
  ptime pt(not_a_date_time);
  time_period tp(pt, td);
  // exceptions for failure_tests
  std::ios_base::failure e_failure("default");

  /* test ptime, time_duration, time_period.
   * test all formats.
   * test for bad input.
   * test special values.
   * do not test custom names (done in gregorian).
   *
   * format flags to test H,M,S,s,F,f,j
   */

  // default format tests: time_duration, ptime
  std::istringstream iss("09:59:01.321987654321 2005-Jan-15 10:15:03.123456789123");
  iss >> td;
  iss >> pt;
#if defined(BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
  check_equal("Default format time_duration", td, time_duration(9,59,1,321987654));
  check_equal("Default format ptime", pt, ptime(date(2005,01,15),time_duration(10,15,3,123456789)));
#else
  check_equal("Default format time_duration", td, time_duration(9,59,1,321987));
  check_equal("Default format ptime", pt, ptime(date(2005,01,15),time_duration(10,15,3,123456)));
#endif

  // test all flags that appear in time_input_facet
  iss.str("12:34:56 2005-Jan-15 12:34:56");
  iss >> td;
  iss >> pt;
  check_equal("Default format time_duration no frac_sec", td, time_duration(12,34,56));
  // the following test insures %F parsing stops at the appropriate point
  check_equal("Default format ptime", pt, ptime(date(2005,01,15),time_duration(12,34,56)));

  iss.str("14:13:12 extra stuff"); // using default %H:%M:%S%F format
  iss >> td;
  check_equal("Default frac_sec format time_duration", td, time_duration(14,13,12));

  time_input_facet* facet = new time_input_facet();
  std::locale loc(std::locale::classic(), facet);
  facet->time_duration_format("%H:%M:%S""%f");
  iss.imbue(loc);

  iss.str("14:13:12.0 extra stuff");
  iss >> td;
  check_equal("Required-frac_sec format time_duration", td, time_duration(14,13,12));

  iss.str("12");
  facet->time_duration_format("%H");
  iss >> td;
  check_equal("Hours format", td, hours(12));

  iss.str("05");
  facet->time_duration_format("%M");
  iss >> td;
  check_equal("Minutes format", td, minutes(5));

  iss.str("45");
  facet->time_duration_format("%S");
  iss >> td;
  check_equal("Seconds w/o frac_sec format", td, seconds(45));

  iss.str("10.01");
  facet->time_duration_format("%s");
  iss >> td;
#if defined(BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
  check_equal("Seconds w/ frac_sec format", td, time_duration(0,0,10,10000000));
#else
  check_equal("Seconds w/ frac_sec format", td, time_duration(0,0,10,10000));
#endif

  iss.str("2005-105T23:59");
  facet->format("%Y-%jT%H:%M"); // extended ordinal format
  iss >> pt;
  check_equal("Extended Ordinal format", pt, ptime(date(2005,4,15),time_duration(23,59,0)));

  /* this is not implemented yet. The flags: %I & %p are not parsed
  iss.str("2005-Jun-14 03:15:00 PM");
  facet->format("%Y-%b-%d %I:%M:%S %p");
  iss >> pt;
  check_equal("12 hour time format (AM/PM)", pt, ptime(date(2005,6,14),time_duration(15,15,0)));
  */

  iss.str("2005-Jun-14 15:15:00 %d");
  facet->format("%Y-%b-%d %H:%M:%S %%d");
  iss >> pt;
  check_equal("Literal '%' in format", pt, ptime(date(2005,6,14),time_duration(15,15,0)));
  iss.str("15:15:00 %d");
  facet->time_duration_format("%H:%M:%S %%d");
  iss >> td;
  check_equal("Literal '%' in time_duration format", td, time_duration(15,15,0));
  iss.str("2005-Jun-14 15:15:00 %14");
  facet->format("%Y-%b-%d %H:%M:%S %%%d"); // %% => % & %d => day_of_month
  iss >> pt;
  check_equal("Multiple literal '%'s in format", pt, ptime(date(2005,6,14),time_duration(15,15,0)));
  iss.str("15:15:00 %15");
  facet->time_duration_format("%H:%M:%S %%%M");
  iss >> td;
  check_equal("Multiple literal '%'s in time_duration format", td, time_duration(15,15,0));

  { /****** iso format tests (and custom 'scrunched-together formats) ******/
    time_input_facet *facet = new time_input_facet();
    facet->set_iso_format();
    facet->time_duration_format("%H""%M""%S""%F"); // iso format
    std::stringstream ss;
    ss.imbue(std::locale(std::locale::classic(), facet));
    ptime pt(not_a_date_time);
    time_duration td(not_a_date_time);
    date d(2002,Oct,17);
#if defined(BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
    time_duration td2(23,12,17,123450000);
#else
    time_duration td2(23,12,17,123450);
#endif
    ptime result(d, td2);

    ss.str("20021017T231217.12345");
    ss >> pt;
    check_equal("iso_format ptime", pt, result);
    ss.str("");
    facet->set_iso_extended_format();
    ss.str("2002-10-17 23:12:17.12345");
    ss >> pt;
    check_equal("iso_extended_format ptime", pt, result);
    ss.str("");
    ss.str("231217.12345");
    ss >> td;
    check_equal("iso_format time_duration", td, td2);
    ss.str("");
    ss.str("-infinity");
    ss >> td;
    check_equal("iso_format time_duration (special_value)",
        td, time_duration(neg_infin));
    ss.str("");
    // the above tests prove correct parsing of time values in these formats.
    // these tests show they also handle special_values & exceptions properly
    time_duration nadt(not_a_date_time);
    ss.exceptions(std::ios_base::failbit); // we need exceptions turned on here
    int count = 0;
    try {
      facet->time_duration_format("%H""%M""%S""%F");
      ss.str("not-a-date-time");
      ++count;
      ss >> td;
      check_equal("special value w/ hours flag", td, nadt);
      ss.str("");
      facet->time_duration_format("%M""%S""%F");
      ss.str("not-a-date-time");
      ++count;
      ss >> td;
      check_equal("special value w/ minutes flag", td, nadt);
      ss.str("");
      facet->time_duration_format("%S""%F");
      ss.str("not-a-date-time");
      ++count;
      ss >> td;
      check_equal("special value w/ seconds flag", td, nadt);
      ss.str("");
      facet->time_duration_format("%s");
      ss.str("not-a-date-time");
      ++count;
      ss >> td;
      check_equal("special value w/ sec w/frac_sec (always) flag", td, nadt);
      ss.str("");
      facet->time_duration_format("%f");
      ss.str("not-a-date-time");
      ++count;
      ss >> td;
      check_equal("special value w/ frac_sec (always) flag", td, nadt);
      ss.str("");
    }
    catch(...) {
      // any exception is a failure
      std::stringstream msg;
      msg << "special_values with scrunched formats failed at test" << count;
      check(msg.str(), false);
    }
    // exception tests
    std::ios_base::failure exc("failure");
    check("failure test w/ hours flag",
        failure_test(td, "bad_input", exc, new time_input_facet("%H""%M""%S""%F")));
    check("silent failure test w/ hours flag",
        failure_test(td, "bad_input", new time_input_facet("%H""%M""%S""%F")));
    check("failure test w/ minute flag",
        failure_test(td, "bad_input", exc, new time_input_facet("%M""%S""%F")));
    check("silent failure test w/ minute flag",
        failure_test(td, "bad_input", new time_input_facet("%M""%S""%F")));
    check("failure test w/ second flag",
        failure_test(td, "bad_input", exc, new time_input_facet("%S""%F")));
    check("silent failure test w/ second flag",
        failure_test(td, "bad_input", new time_input_facet("%S""%F")));
    check("failure test w/ sec w/frac (always) flag",
        failure_test(td, "bad_input", exc, new time_input_facet("%s")));
    check("silent failure test w/ sec w/frac (always) flag",
        failure_test(td, "bad_input", new time_input_facet("%s")));
    check("failure test w/ frac_sec flag",
        failure_test(td, "bad_input", exc, new time_input_facet("%f")));
    check("silent failure test w/ frac_sec flag",
        failure_test(td, "bad_input", new time_input_facet("%f")));

  }
  // special_values tests. prove the individual flags catch special_values
  // NOTE: these flags all by themselves will not parse a complete ptime,
  // these are *specific* special_values tests
  iss.str("+infinity -infinity");
  facet->format("%H");
  facet->time_duration_format("%H");
  iss >> pt;
  iss >> td;
  check_equal("Special value: ptime %H flag", pt, ptime(pos_infin));
  check_equal("Special value: time_duration %H flag", td, time_duration(neg_infin));

  iss.str("not-a-date-time +infinity");
  facet->format("%M");
  facet->time_duration_format("%M");
  iss >> pt;
  iss >> td;
  check_equal("Special value: ptime %M flag", pt, ptime(not_a_date_time));
  check_equal("Special value: time_duration %M flag", td, time_duration(pos_infin));

  iss.str("-infinity not-a-date-time ");
  facet->format("%S");
  facet->time_duration_format("%S");
  iss >> pt;
  iss >> td;
  check_equal("Special value: ptime %S flag", pt, ptime(neg_infin));
  check_equal("Special value: time_duration %S flag", td, time_duration(not_a_date_time));

  iss.str("+infinity -infinity");
  facet->format("%s");
  facet->time_duration_format("%s");
  iss >> pt;
  iss >> td;
  check_equal("Special value: ptime %s flag", pt, ptime(pos_infin));
  check_equal("Special value: time_duration %s flag", td, time_duration(neg_infin));

  iss.str("not-a-date-time +infinity");
  facet->format("%j");
  facet->time_duration_format("%f");
  iss >> pt;
  iss >> td;
  check_equal("Special value: ptime %j flag", pt, ptime(not_a_date_time));
  check_equal("Special value: time_duration %f flag", td, time_duration(pos_infin));

  // time_period tests - the time_period_parser is thoroughly tested in gregorian tests
  // default period format is closed range so last ptime is included in peiod
  iss.str("[2005-Jan-01 00:00:00/2005-Dec-31 23:59:59]");
  facet->format(time_input_facet::default_time_input_format); // reset format
  iss >> tp;
  check("Time period, default formats", 
      (tp.begin() == ptime(date(2005,1,1),hours(0))) &&
      (tp.last() == ptime(date(2005,12,31),time_duration(23,59,59))) &&
      (tp.end() == ptime(date(2005,12,31),time_duration(23,59,59,1))) );
  {
    std::stringstream ss;
    ptime pt(not_a_date_time);
    ptime pt2 = second_clock::local_time();
    ptime pt3(neg_infin);
    ptime pt4(pos_infin);
    time_period tp(pt2, pt); // ptime/nadt
    time_period tp2(pt, pt); // nadt/nadt
    time_period tp3(pt3, pt4);
    ss << tp;
    ss >> tp2;
    check_equal("Special values period (reversibility test)", tp, tp2);
    ss.str("[-infinity/+infinity]");
    ss >> tp2;
    check_equal("Special values period (infinities)", tp3, tp2);
  }

  // Failure tests
  // faliure tests for date elements tested in gregorian tests
  time_input_facet* facet2 = new time_input_facet();
  facet2->time_duration_format("%H:%M:%S""%f");
  check("Failure test: Missing frac_sec with %f flag (w/exceptions)",
        failure_test(td, "14:13:12 extra stuff", e_failure, facet2));
  time_input_facet* facet3 = new time_input_facet();
  facet3->time_duration_format("%H:%M:%S""%f");
  check("Failure test: Missing frac_sec with %f flag (no exceptions)",
        failure_test(td, "14:13:12 extra stuff", facet3));

  // Reversable format tests
  time_duration td_io(10,11,12,1234567);
  ptime pt_io(date(2004,03,15), td_io);
  time_facet* otp_facet = new time_facet();
  time_input_facet* inp_facet = new time_input_facet();
  std::stringstream ss;
  std::locale loc2(std::locale::classic(), otp_facet);
  ss.imbue(std::locale(loc2, inp_facet)); // imbue locale containing both facets

  ss.str("");
  ss << pt_io; // stream out pt_io
  ss >> pt;
  check_equal("Stream out one ptime then into another: default format", pt_io, pt);
  ss.str("");
  ss << td_io; // stream out td_io
  ss >> td;
  check_equal("Stream out one time_duration then into another: default format", td_io, td);

  td_io = time_duration(1,29,59); // no frac_sec, default format has %F
  pt_io = ptime(date(2004,2,29), td_io); // leap year
  ss.str("");
  ss << pt_io; // stream out pt_io
  ss >> pt;
  check_equal("Stream out one ptime then into another: default format", pt_io, pt);
  ss.str("");
  ss << td_io; // stream out td_io
  ss >> td;
  check_equal("Stream out one time_duration then into another: default format", td_io, td);

  td_io = time_duration(13,05,0); // no seconds as the next formats won't use them
  pt_io = ptime(date(2004,2,29), td_io); // leap year
  otp_facet->format("%Y-%jT%H:%M"); // extended ordinal
  inp_facet->format("%Y-%jT%H:%M");
  ss.str("");
  ss << pt_io; // stream out pt_io
  ss >> pt;
  check_equal("Stream out one ptime then into another: extended ordinal format", pt_io, pt);

  otp_facet->format("Time: %H:%M, Date: %B %d, %Y"); // custom format with extra words
  inp_facet->format("Time: %H:%M, Date: %B %d, %Y");
  ss.str("");
  ss << pt_io; // stream out pt_io
  ss >> pt;
  check_equal("Stream out one ptime then into another: custom format (" + ss.str() + ")", pt_io, pt);

  {
    // fully parameterized constructor - compile only test, all other features tested in gregorian
    boost::date_time::format_date_parser<date, char> fdp("%Y-%b-%d", std::locale::classic());
    special_values_parser svp; // default constructor
    period_parser pp; // default constructor
    boost::date_time::date_generator_parser<date, char> dgp; // default constructor
    time_input_facet tif("%Y-%m-%d %H:%M:%s", fdp, svp, pp, dgp);
  }
#endif // USE_DATE_TIME_PRE_1_33_FACET_IO

}



int main(){

  try {  //wrap all the tests -- we don't expect an exception
    do_all_tests();
  }
  catch(std::exception& e) {
    std::string failure("std::exception caught: ");
    failure += e.what();
    check(failure, false);
  }
  catch(...) {
    check("Unknown exception caught -- failing", false);
  }
  return printTestStats();
}

