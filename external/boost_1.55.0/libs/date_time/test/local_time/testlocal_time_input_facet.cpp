/* Copyright (c) 2005 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */

#include "boost/date_time/local_time/local_time.hpp"
#include "../testfrmwk.hpp"
#include <iostream>
#include <sstream>
#include <string>

// for tests that are expected to fail and throw exceptions
template<class temporal_type, class exception_type>
bool failure_test(temporal_type component,
                  const std::string& input,
                  exception_type const& /*except*/,
                  boost::local_time::local_time_input_facet* facet)
{
  using namespace boost::local_time;
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
                  boost::local_time::local_time_input_facet* facet)
{
  using namespace boost::local_time;
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

int main() {
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  using namespace boost::local_time;
  time_zone_ptr null_zone;
  local_date_time ldt1(not_a_date_time, null_zone);

  // verify wide stream works, thorough tests done in narrow stream
#if !defined(BOOST_NO_STD_WSTRING)
  {
    std::wstringstream ws;
    ws.str(L"2005-Feb-15 12:15:00 EST-05EDT,M4.1.0,M10.5.0");
    ws >> ldt1;
    check("Wide stream, Eastern US, daylight savings, winter, minimal input",
          ldt1.local_time() == ptime(date(2005,2,15), time_duration(12,15,0)));
    check("Wide stream, Eastern US, daylight savings, winter, minimal input",
          ldt1.utc_time() == ptime(date(2005,2,15), time_duration(17,15,0)));
    check("Wide stream, Eastern US, daylight savings, winter, minimal input", !ldt1.is_dst());
    ws.str(L"");
    wlocal_time_input_facet* wfacet = new wlocal_time_input_facet(L"%m/%d/%y %ZP");
    std::locale loc(std::locale::classic(), wfacet);
    ws.imbue(loc);
    ws.str(L"10/31/04 PST-08PDT,M4.1.0,M10.5.0"); // midnight on end transition day, still in dst
    ws >> ldt1;
    std::wcout << ldt1.local_time() << std::endl;
    check("Wide stream, Eastern US, daylight savings, winter, custom format",
          ldt1.local_time() == ptime(date(2004,10,31), time_duration(0,0,0)));
    check("Wide stream, Eastern US, daylight savings, winter, custom format",
          ldt1.utc_time() == ptime(date(2004,10,31), time_duration(7,0,0)));
    check("Wide stream, Eastern US, daylight savings, winter, custom format", ldt1.is_dst());
  }
#endif // BOOST_NO_STD_WSTRING

  std::stringstream ss;
  ss.str("2005-Feb-25 12:15:00 EST-05EDT,M4.1.0,M10.5.0");
  ss >> ldt1;
  check("Eastern US, daylight savings, winter, minimal input",
        ldt1.local_time() == ptime(date(2005,2,25), time_duration(12,15,0)));
  check("Eastern US, daylight savings, winter, minimal input",
        ldt1.utc_time() == ptime(date(2005,2,25), time_duration(17,15,0)));
  check("Eastern US, daylight savings, winter, minimal input", !ldt1.is_dst());
  ss.str("");
  
  ss.str("2005-Aug-25 12:15:00 EST-05EDT,M4.1.0,M10.5.0");
  ss >> ldt1;
  check("Eastern US, daylight savings, summer, minimal input",
        ldt1.local_time() == ptime(date(2005,8,25), time_duration(12,15,0)));
  check("Eastern US, daylight savings, summer, minimal input",
        ldt1.utc_time() == ptime(date(2005,8,25), time_duration(16,15,0)));
  check("Eastern US, daylight savings, summer, minimal input", ldt1.is_dst());
  ss.str("");
  
  ss.str("2005-Apr-03 01:15:00 EST-05EDT,M4.1.0,M10.5.0");
  ss >> ldt1;
  check("Eastern US, daylight savings, transition point", !ldt1.is_dst());
  ldt1 += hours(1);
  check("Eastern US, daylight savings, transition point", ldt1.is_dst());
  ss.str("");
  ss.str("2005-Apr-03 01:15:00 EST-05EDT,93,303");
  ss >> ldt1;
  check("Eastern US, daylight savings, transition point", !ldt1.is_dst());
  ldt1 += hours(1);
  check("Eastern US, daylight savings, transition point", ldt1.is_dst());
  ss.str("");
  
  ss.str("2005-Oct-30 00:15:00 EST-05EDT,M4.1.0,M10.5.0");
  ss >> ldt1;
  check("Eastern US, daylight savings, transition point", ldt1.is_dst());
  ldt1 += hours(1);
  check("Eastern US, daylight savings, transition point", ldt1.is_dst());
  ldt1 += hours(1);
  check("Eastern US, daylight savings, transition point", !ldt1.is_dst());
  ss.str("");
  ss.str("2005-Oct-30 00:15:00 EST-05EDT,93,303");
  ss >> ldt1;
  check("Eastern US, daylight savings, transition point", ldt1.is_dst());
  ldt1 += hours(1);
  check("Eastern US, daylight savings, transition point", ldt1.is_dst());
  ldt1 += hours(1);
  check("Eastern US, daylight savings, transition point", !ldt1.is_dst());
  ss.str("");
  
  ss.str("2005-Aug-25 12:15:00 MST-07");
  ss >> ldt1;
  check("Mountain US, no daylight savings",
        ldt1.local_time() == ptime(date(2005,8,25), time_duration(12,15,0)));
  check("Mountain US, no daylight savings",
        ldt1.utc_time() == ptime(date(2005,8,25), time_duration(19,15,0)));
  check("Mountain US, no daylight savings", !ldt1.is_dst());
  ss.str("");

  // insure input & output formats match
  local_time_facet* out_facet = 
    new local_time_facet(local_time_input_facet::default_time_input_format);
  std::locale loc(std::locale::classic(), out_facet);
  ss.imbue(loc);
  time_zone_ptr syd_tz(new posix_time_zone("EST+10EST,M10.5.0,M3.5.0/03:00"));
  ptime pt(date(2005,6,12), hours(0));
  local_date_time ldt2(pt, syd_tz);
  ss << ldt2;
  ss >> ldt1;
  check("Output as input makes match", ldt1 == ldt2);
  check("Output as input makes match", 
      ldt1.zone()->dst_local_start_time(2004) == ldt2.zone()->dst_local_start_time(2004));
  ss.str("");
  
  time_zone_ptr f_tz(new posix_time_zone("FST+03FDT,90,300"));
  ldt2 = local_date_time(ptime(date(2005,6,12), hours(0)), f_tz);
  ss << ldt2;
  ss >> ldt1;
  check("Output as input makes match", ldt1 == ldt2);
  check("Output as input makes match", 
      ldt1.zone()->dst_local_start_time(2004) == ldt2.zone()->dst_local_start_time(2004));
  ss.str("");

  // missing input & wrong format tests
  ss.str("2005-Oct-30 00:15:00");
  ss >> ldt1;
  check("Missing time_zone spec makes UTC", ldt1.zone_as_posix_string() == std::string("UTC+00"));
  check("Missing time_zone spec makes UTC", ldt1.utc_time() == ldt1.local_time());
  ss.str("");
  {
    std::istringstream iss("2005-Aug-25 12:15:00 MST-07");
    local_time_input_facet* f = new local_time_input_facet("%Y-%b-%d %H:%M:%S %z");
    std::locale loc(std::locale::classic(), f);
    iss.imbue(loc);
    iss >> ldt1;
    check("Wrong format flag makes UTC", ldt1.zone_as_posix_string() == std::string("UTC+00"));
    check("Wrong format flag makes UTC", ldt1.utc_time() == ldt1.local_time());
  }
  
 
  // failure tests: (posix_time_zone) bad_offset, bad_adjustment, 
  // (local_date_time) ambiguous_result, time_label_invalid, 
  // time/date failures already tested
  ambiguous_result amb_ex("default");
  time_label_invalid inv_ex("default");
  check("Failure test ambiguous time label (w/exceptions)", 
        failure_test(ldt1,
                     "2005-Oct-30 01:15:00 EST-05EDT,M4.1.0,M10.5.0",
                     amb_ex, 
                     new local_time_input_facet()));
  check("Failure test ambiguous time label (no exceptions)", 
        failure_test(ldt1,
                     "2005-Oct-30 01:15:00 EST-05EDT,M4.1.0,M10.5.0",
                     new local_time_input_facet()));
  check("Failure test ambiguous time label (w/exceptions)", 
        failure_test(ldt1,
                     "2005-Oct-30 01:15:00 EST-05EDT,93,303",
                     amb_ex, 
                     new local_time_input_facet()));
  check("Failure test ambiguous time label (no exceptions)", 
        failure_test(ldt1,
                     "2005-Oct-30 01:15:00 EST-05EDT,93,303",
                     new local_time_input_facet()));
  check("Failure test invalid time label (w/exceptions)", 
        failure_test(ldt1,
                     "2005-Apr-03 02:15:00 EST-05EDT,M4.1.0,M10.5.0",
                     inv_ex, 
                     new local_time_input_facet()));
  check("Failure test invalid time label (no exceptions)", 
        failure_test(ldt1,
                     "2005-Apr-03 02:15:00 EST-05EDT,M4.1.0,M10.5.0",
                     new local_time_input_facet()));
  check("Failure test invalid time label (w/exceptions)", 
        failure_test(ldt1,
                     "2005-Apr-03 02:15:00 EST-05EDT,93,303",
                     inv_ex, 
                     new local_time_input_facet()));
  check("Failure test invalid time label (no exceptions)", 
        failure_test(ldt1,
                     "2005-Apr-03 02:15:00 EST-05EDT,93,303",
                     new local_time_input_facet()));


  return printTestStats();
}
