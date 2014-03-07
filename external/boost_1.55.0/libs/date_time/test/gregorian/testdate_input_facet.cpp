/* Copyright (c) 2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2013-10-15 08:22:02 -0700 (Tue, 15 Oct 2013) $
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef USE_DATE_TIME_PRE_1_33_FACET_IO
// for tests that are expected to fail and throw exceptions
template<class temporal_type, class exception_type>
bool failure_test(temporal_type component,
                  const std::string& input,
                  exception_type const& /*except*/,
                  boost::gregorian::date_input_facet* facet)
{
  using namespace boost::gregorian;
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
                  boost::gregorian::date_input_facet* facet)
{
  using namespace boost::gregorian;
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

#endif



int main(){
#ifndef  USE_DATE_TIME_PRE_1_33_FACET_IO
  using namespace boost::gregorian;

  {
    // verify no extra character are consumed
    greg_month m(1);
    std::stringstream ss("Mar.");
    std::istreambuf_iterator<char> sitr(ss), str_end;

    date_input_facet* f = new date_input_facet();
    f->get(sitr, str_end, ss, m);
    check("No extra characters consumed", m == greg_month(Mar) && *sitr == '.');
  }
 
  // set up initial objects
  date d(not_a_date_time);
  days dd(not_a_date_time);
  greg_month m(1);
  greg_weekday gw(0);
  greg_day gd(1);
  greg_year gy(2000);
  // exceptions for failure_tests
  std::ios_base::failure e_failure("default");
  bad_month e_bad_month;
  bad_year e_bad_year;
  bad_day_of_month e_bad_day_of_month;
  bad_weekday e_bad_weekday;
  bad_day_of_year e_bad_day_of_year;

  // default format tests: date, days, month, weekday, day, year
  std::istringstream iss("2005-Jan-15 21 Feb Tue 4 2002");
  iss >> d;
  check_equal("Default format date", d, date(2005,Jan,15));
  iss >> dd;
  check_equal("Default (only) format positive days", dd, days(21));
  iss >> m;
  check_equal("Default format month", m, greg_month(2));
  iss >> gw;
  check_equal("Default format weekday", gw, greg_weekday(2));
  iss >> gd;
  check_equal("Default (only) format day of month", gd, greg_day(4));
  iss >> gy;
  check_equal("Default format year", gy, greg_year(2002));
  // failure tests
  check("Input Misspelled in year (date) w/exceptions", 
      failure_test(d, "205-Jan-15", e_bad_year, new date_input_facet()));
  check("Input Misspelled in year (date) no-exceptions", 
      failure_test(d, "205-Jan-15", new date_input_facet()));
  check("Input Misspelled in month (date) w/exceptions", 
      failure_test(d, "2005-Jsn-15", e_bad_month, new date_input_facet()));
  check("Input Misspelled in month (date) no-exceptions", 
      failure_test(d, "2005-Jsn-15", new date_input_facet()));
  check("Input Misspelled in day (date) w/exceptions", 
      failure_test(d, "2005-Jan-51", e_bad_day_of_month, new date_input_facet()));
  check("Input Misspelled in day (date) no-exceptions", 
      failure_test(d, "2005-Jan-51", new date_input_facet()));
  check("Input Misspelled greg_weekday w/exceptions", 
      failure_test(gw, "San", e_bad_weekday, new date_input_facet()));
  check("Input Misspelled greg_weekday no-exceptions", 
      failure_test(gw, "San", new date_input_facet()));
  check("Input Misspelled month w/exceptions", 
      failure_test(m, "Jsn", e_bad_month, new date_input_facet()));
  check("Input Misspelled month no-exceptions", 
      failure_test(m, "Jsn", new date_input_facet()));
  check("Bad Input greg_day w/exceptions", 
      failure_test(gd, "Sun", e_bad_day_of_month, new date_input_facet()));
  check("Bad Input greg_day no-exceptions", 
      failure_test(gd, "Sun", new date_input_facet()));
  check("Input Misspelled greg_year w/exceptions", 
      failure_test(gy, "205", e_bad_year, new date_input_facet()));
  check("Input Misspelled greg_year no-exceptions", 
      failure_test(gy, "205", new date_input_facet()));

  // change to full length names, iso date format, and 2 digit year
  date_input_facet* facet = new date_input_facet();
  facet->set_iso_format();
  facet->month_format("%B");
  facet->weekday_format("%A");
  facet->year_format("%y");
  iss.str("20050115 -55 February Tuesday 02");
  iss.imbue(std::locale(std::locale::classic(), facet));

  iss >> d;
  check_equal("ISO format date", d, date(2005,Jan,15));
  iss >> dd;
  check_equal("Default (only) format negative days", dd, days(-55));
  iss >> m;
  check_equal("Full format month", m, greg_month(2));
  iss >> gw;
  check_equal("Full format weekday", gw, greg_weekday(2));
  iss >> gy;
  check_equal("2 digit format year", gy, greg_year(2002));

  date_input_facet* f1 = new date_input_facet();
  date_input_facet* f2 = new date_input_facet();
  f1->set_iso_format();
  f2->set_iso_format();
  check("Missing digit(s) in ISO string", failure_test(d,"2005071", f1));
  check("Missing digit(s) in ISO string", 
        failure_test(d,"2005071", e_bad_day_of_month, f2));

  { // literal % in format tests
    date d(not_a_date_time);
    greg_month m(1);
    greg_weekday gw(0);
    greg_year y(1400);
    date_input_facet* f = new date_input_facet("%%d %Y-%b-%d");
    std::stringstream ss;
    ss.imbue(std::locale(ss.getloc(), f));

    ss.str("%d 2005-Jun-14");
    ss >> d;
    check_equal("Literal '%' in date format", d, date(2005,Jun,14));
    f->format("%%%d %Y-%b-%d");
    ss.str("%14 2005-Jun-14");
    ss >> d;
    check_equal("Multiple literal '%'s in date format", d, date(2005,Jun,14));

    f->month_format("%%b %b");
    ss.str("%b Jun");
    ss >> m;
    check_equal("Literal '%' in month format", m, greg_month(6));
    f->month_format("%%%b");
    ss.str("%Jun");
    ss >> m;
    check_equal("Multiple literal '%'s in month format", m, greg_month(6));

    f->weekday_format("%%a %a");
    ss.str("%a Tue");
    ss >> gw;
    check_equal("Literal '%' in weekday format", gw, greg_weekday(2));
    f->weekday_format("%%%a");
    ss.str("%Tue");
    ss >> gw;
    check_equal("Multiple literal '%'s in weekday format", gw, greg_weekday(2));

    f->year_format("%%Y %Y");
    ss.str("%Y 2005");
    ss >> y;
    check_equal("Literal '%' in year format", y, greg_year(2005));
    f->year_format("%%%Y");
    ss.str("%2005");
    ss >> y;
    check_equal("Multiple literal '%'s in year format", y, greg_year(2005));

    f->year_format("%Y%");
    ss.str("2005%");
    ss >> y;
    check_equal("Trailing'%'s in year format", y, greg_year(2005));
  }

  // All days, month, weekday, day, and year formats have been tested
  // begin testing other date formats
  facet->set_iso_extended_format();
  iss.str("2005-01-15");
  iss >> d;
  check_equal("ISO Extended format date", d, date(2005,Jan,15));

  facet->format("%B %d, %Y");
  iss.str("March 15, 2006");
  iss >> d;
  check_equal("Custom date format: \"%B %d, %Y\" => 'March 15, 2006'",
      d, date(2006,Mar,15));

  facet->format("%Y-%j"); // Ordinal format ISO8601(2000 sect 5.2.2.1 extended)
  iss.str("2006-074");
  iss >> d;
  check_equal("Custom date format: \"%Y-%j\" => '2006-074'",
      d, date(2006,Mar,15));
  check("Bad input Custom date format: \"%Y-%j\" => '2006-74' (w/exceptions)",
      failure_test(d, "2006-74", e_bad_day_of_year, facet));
  check("Bad input Custom date format: \"%Y-%j\" => '2006-74' (no exceptions)",
      failure_test(d, "2006-74", facet));

  // date_period tests

  // A date_period is constructed with an open range. So the periods
  // [2000-07--04/2000-07-25) <-- open range
  // And
  // [2000-07--04/2000-07-24] <-- closed range
  // Are equal
  date begin(2002, Jul, 4);
  days len(21);
  date_period dp(date(2000,Jan,1), days(1));
  iss.str("[2002-07-04/2002-07-24]");
  facet->set_iso_extended_format();
  iss >> dp;
  check_equal("Default period (closed range)", dp, date_period(begin,len));
  {
    std::stringstream ss;
    date d(not_a_date_time);
    date d2 = day_clock::local_day();
    date d3(neg_infin);
    date d4(pos_infin);
    date_period dp(d2, d); // date/nadt
    date_period dp2(d, d); // nadt/nadt
    date_period dp3(d3, d4);
    ss << dp;
    ss >> dp2;
    check_equal("Special values period (reversibility test)", dp, dp2);
    ss.str("[-infinity/+infinity]");
    ss >> dp2;
    check_equal("Special values period (infinities)", dp3, dp2);
  }


  // open range
  period_parser pp(period_parser::AS_OPEN_RANGE);
  iss.str("[2002-07-04/2002-07-25)");
  facet->period_parser(pp);
  iss >> dp;
  check_equal("Open range period", dp, date_period(begin,len));
  // custom period delimiters
  pp.delimiter_strings(" to ", "from ", " exclusive", " inclusive");
  iss.str("from 2002-07-04 to 2002-07-25 exclusive");
  facet->period_parser(pp);
  iss >> dp;
  check_equal("Open range period - custom delimiters", dp, date_period(begin,len));
  pp.range_option(period_parser::AS_CLOSED_RANGE);
  iss.str("from 2002-07-04 to 2002-07-24 inclusive");
  facet->period_parser(pp);
  iss >> dp;
  check_equal("Closed range period - custom delimiters", dp, date_period(begin,len));


  // date_generator tests

  // date_generators use formats contained in the 
  // date_input_facet for weekdays and months
  // reset month & weekday formats to defaults
  facet->month_format("%b");
  facet->weekday_format("%a");

  partial_date pd(1,Jan);
  nth_kday_of_month nkd(nth_kday_of_month::first, Sunday, Jan);
  first_kday_of_month fkd(Sunday, Jan);
  last_kday_of_month lkd(Sunday, Jan);
  first_kday_before fkb(Sunday);
  first_kday_after fka(Sunday);
  // using default date_generator_parser "nth_strings"
  iss.str("29 Feb");
  iss >> pd;
  // Feb-29 is a valid date_generator, get_date() will fail in a non-leap year
  check_equal("Default strings, partial_date",
      pd.get_date(2004), date(2004,Feb,29));
  iss.str("second Mon of Mar");
  iss >> nkd; 
  check_equal("Default strings, nth_day_of_the_week_in_month",
      nkd.get_date(2004), date(2004,Mar,8));
  iss.str("first Tue of Apr");
  iss >> fkd;
  check_equal("Default strings, first_day_of_the_week_in_month",
      fkd.get_date(2004), date(2004,Apr,6));
  iss.str("last Wed of May");
  iss >> lkd;
  check_equal("Default strings, last_day_of_the_week_in_month",
      lkd.get_date(2004), date(2004,May,26));
  iss.str("Thu before");
  iss >> fkb;
  check_equal("Default strings, first_day_of_the_week_before",
      fkb.get_date(date(2004,Feb,8)), date(2004,Feb,5));
  iss.str("Fri after");
  iss >> fka;
  check_equal("Default strings, first_day_of_the_week_after",
      fka.get_date(date(2004,Feb,1)), date(2004,Feb,6));
  // failure tests
  check("Incorrect elements (date_generator) w/exceptions", // after/before type mixup
      failure_test(fkb, "Fri after", e_failure, new date_input_facet()));
  check("Incorrect elements (date_generator) no exceptions", // after/before type mixup
      failure_test(fkb, "Fri after", new date_input_facet()));
  check("Incorrect elements (date_generator) w/exceptions", // first/last type mixup
      failure_test(lkd, "first Tue of Apr", e_failure, new date_input_facet()));
  check("Incorrect elements (date_generator) no exceptions", // first/last type mixup
      failure_test(lkd, "first Tue of Apr", new date_input_facet()));
  check("Incorrect elements (date_generator) w/exceptions", // 'in' is wrong
      failure_test(nkd, "second Mon in Mar", e_failure, new date_input_facet()));
  check("Incorrect elements (date_generator) no exceptions", // 'in' is wrong
      failure_test(nkd, "second Mon in Mar", new date_input_facet()));

  // date_generators - custom element strings
  facet->date_gen_element_strings("1st","2nd","3rd","4th","5th","final","prior to","past","in");
  iss.str("3rd Sat in Jul");
  iss >> nkd;
  check_equal("Custom strings, nth_day_of_the_week_in_month",
      nkd.get_date(2004), date(2004,Jul,17));
  iss.str("1st Wed in May");
  iss >> fkd;
  check_equal("Custom strings, first_day_of_the_week_in_month",
      fkd.get_date(2004), date(2004,May,5));
  iss.str("final Tue in Apr");
  iss >> lkd;
  check_equal("Custom strings, last_day_of_the_week_in_month",
      lkd.get_date(2004), date(2004,Apr,27));
  iss.str("Fri prior to");
  iss >> fkb;
  check_equal("Custom strings, first_day_of_the_week_before",
      fkb.get_date(date(2004,Feb,8)), date(2004,Feb,6));
  iss.str("Thu past");
  iss >> fka;
  check_equal("Custom strings, first_day_of_the_week_after",
      fka.get_date(date(2004,Feb,1)), date(2004,Feb,5));

  // date_generators - special case with empty element string
  /* Doesn't work. Empty string returns -1 from string_parse_tree 
   * because it attempts to match the next set of characters in the 
   * stream to the wrong element. Ex. It attempts to match "Mar" to 
   * the 'of' element in the test below.
   *
  facet->date_gen_element_strings("1st","2nd","3rd","4th","5th","final","prior to","past",""); // the 'of' string is an empty string
  iss.str("final Mon Mar");
  iss >> lkd;
  check_equal("Special case, empty element string",
      lkd.get_date(2005), date(2005,Mar,28));
      */


  // special values tests (date and days only)
  iss.str("minimum-date-time +infinity");
  iss >> d;
  iss >> dd;
  check_equal("Special values, default strings, min_date_time date",
      d, date(min_date_time));
  check_equal("Special values, default strings, pos_infin days",
      dd, days(pos_infin));
  iss.str("-infinity maximum-date-time");
  iss >> d;
  iss >> dd;
  check_equal("Special values, default strings, neg_infin date",
      d, date(neg_infin));
  check_equal("Special values, default strings, max_date_time days",
      dd, days(max_date_time));
  iss.str("not-a-date-time");
  iss >> d;
  check_equal("Special values, default strings, not_a_date_time date",
      d, date(not_a_date_time));

  // in addition check that special_value_from_string also works correctly for other special values
  check_equal("Special values, default strings, not_special test",
      special_value_from_string("not_special"), not_special);
  check_equal("Special values, default strings, junk test",
      special_value_from_string("junk"), not_special);

  // special values custom, strings
  special_values_parser svp("NADT", "MINF", "INF", "MINDT", "MAXDT");
  facet->special_values_parser(svp);
  iss.str("MINDT INF");
  iss >> d;
  iss >> dd;
  check_equal("Special values, custom strings, min_date_time date",
      d, date(min_date_time));
  check_equal("Special values, custom strings, pos_infin days",
      dd, days(pos_infin));
  iss.str("MINF MAXDT");
  iss >> d;
  iss >> dd;
  check_equal("Special values, custom strings, neg_infin date",
      d, date(neg_infin));
  check_equal("Special values, custom strings, max_date_time days",
      dd, days(max_date_time));
  iss.str("NADT");
  iss >> dd;
  check_equal("Special values, custom strings, not_a_date_time days",
      dd, days(not_a_date_time));
  // failure test
  check("Misspelled input, special_value date w/exceptions",
      failure_test(d, "NSDT", e_bad_year, new date_input_facet()));
  check("Misspelled input, special_value date no exceptions",
      failure_test(d, "NSDT", new date_input_facet()));
  check("Misspelled input, special_value days w/exceptions",
      failure_test(dd, "NSDT", e_failure, new date_input_facet()));
  check("Misspelled input, special_value days no exceptions",
      failure_test(dd, "NSDT", new date_input_facet()));

  {
    // German names. Please excuse any errors, I don't speak German and
    // had to rely on an on-line translation service.
    // These tests check one of each (at least) from all sets of custom strings

    // create a custom format_date_parser
    std::string m_a[] = {"Jan","Feb","Mar","Apr","Mai",
                         "Jun","Jul","Aug","Sep","Okt","Nov","Dez"};
    std::string m_f[] = {"Januar","Februar","Marz","April",
                         "Mai","Juni","Juli","August",
                         "September","Oktober","November","Dezember"};
    std::string w_a[] = {"Son", "Mon", "Die","Mit", "Don", "Fre", "Sam"};
    std::string w_f[] = {"Sonntag", "Montag", "Dienstag","Mittwoch",
                         "Donnerstag", "Freitag", "Samstag"};
    typedef boost::date_time::format_date_parser<date, char> date_parser;
    date_parser::input_collection_type months_abbrev;
    date_parser::input_collection_type months_full;
    date_parser::input_collection_type wkdays_abbrev;
    date_parser::input_collection_type wkdays_full;
    months_abbrev.assign(m_a, m_a+12);
    months_full.assign(m_f, m_f+12);
    wkdays_abbrev.assign(w_a, w_a+7);
    wkdays_full.assign(w_f, w_f+7);
    date_parser d_parser("%B %d %Y",
                         months_abbrev, months_full,
                         wkdays_abbrev, wkdays_full);

    // create a special_values parser
    special_values_parser sv_parser("NichtDatumzeit",
                                    "Negativ Unendlichkeit",
                                    "Positiv Unendlichkeit",
                                    "Wenigstes Datum",
                                    "Maximales Datum");

    // create a period_parser
    period_parser p_parser; // default will do
    // create date_generator_parser
    typedef boost::date_time::date_generator_parser<date,char> date_gen_parser;
    date_gen_parser dg_parser("Zuerst","Zweitens","Dritt","Viert",
                              "Fünft","Letzt","Vor","Nach","Von");

    // create the date_input_facet
    date_input_facet* de_facet =
      new date_input_facet("%B %d %Y",
                           d_parser,
                           sv_parser,
                           p_parser,
                           dg_parser);
    std::istringstream iss;
    iss.imbue(std::locale(std::locale::classic(), de_facet));
    // June 06 2005, Dec, minimum date, Tues
    iss.str("Juni 06 2005 Dez Wenigstes Datum Die");
    iss >> d;
    iss >> m;
    check_equal("German names: date", d, date(2005, Jun, 6));
    check_equal("German names: month", m, greg_month(Dec));
    iss >> d;
    iss >> gw;
    check_equal("German names: special value date", d, date(min_date_time));
    check_equal("German names: short weekday", gw, greg_weekday(Tuesday));
    de_facet->weekday_format("%A"); // long weekday
    // Tuesday, Second Tuesday of Mar
    iss.str("Dienstag Zweitens Dienstag von Mar");
    iss >> gw;
    iss >> nkd;
    check_equal("German names: long weekday", gw, greg_weekday(Tuesday));
    check_equal("German names, nth_day_of_the_week_in_month",
        nkd.get_date(2005), date(2005,Mar,8));
    // Tuesday after
    iss.str("Dienstag Nach");
    iss >> fka;
    check_equal("German names, first_day_of_the_week_after",
        fka.get_date(date(2005,Apr,5)), date(2005,Apr,12));
  }

  {
    // test name replacement functions

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

    std::copy(&weekday_short_names[0],
              &weekday_short_names[7],
              std::back_inserter(short_weekday_names));
    std::copy(&weekday_long_names[0],
              &weekday_long_names[7],
              std::back_inserter(long_weekday_names));
    std::copy(&month_short_names[0],
              &month_short_names[12],
              std::back_inserter(short_month_names));
    std::copy(&month_long_names[0],
              &month_long_names[12],
              std::back_inserter(long_month_names));

    date d(not_a_date_time);
    date_input_facet* facet = new date_input_facet();
    std::stringstream ss;
    ss.imbue(std::locale(std::locale::classic(), facet));
    facet->short_month_names(short_month_names);
    facet->short_weekday_names(short_weekday_names);
    facet->long_month_names(long_month_names);
    facet->long_weekday_names(long_weekday_names);
    facet->format("%a %b %d, %Y");
    ss.str("day7 *apr* 23, 2005");
    ss >> d;
    check_equal("Short custom names, set via accessor function", d.day_of_week(), greg_weekday(6));
    check_equal("Short custom names, set via accessor function", d.month(), greg_month(4));
    ss.str("");
    ss.str("Sun-0 **April** 24, 2005");
    facet->format("%A %B %d, %Y");
    ss >> d;
    check_equal("Long custom names, set via accessor function", d.day_of_week(), greg_weekday(0));
    check_equal("Long custom names, set via accessor function", d.month(), greg_month(4));

  }
#else
    check("This test is a nop for platforms with USE_DATE_TIME_PRE_1_33_FACET_IO",
          true);
#endif
  return printTestStats();
}

