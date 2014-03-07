#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"
#include <iostream>
#include <locale>

int main() {
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  using namespace boost::local_time;

  tz_database tz_db;  
  try {
    tz_db.load_from_file("../../data/date_time_zonespec.csv");
  }catch(data_not_accessible dna) {
    std::cerr << "Error with time zone data file: " << dna.what() << std::endl;
    exit(EXIT_FAILURE);
  }catch(bad_field_count bfc) {
    std::cerr << "Error with time zone data file: " << bfc.what() << std::endl;
    exit(EXIT_FAILURE);
  }

  time_zone_ptr nyc = tz_db.time_zone_from_region("America/New_York");
  local_date_time ny_time(date(2004, Aug, 30), hours(10), nyc, true);
  
  typedef boost::date_time::time_facet<local_date_time, char> ldt_facet;
  ldt_facet* timefacet = new ldt_facet("%Y-%b-%d %H:%M:%S""%F %Z");
  std::locale loc(std::locale::classic(), timefacet);
  
  std::cout << ny_time << std::endl;
  // 2004-Aug-30 10:00:00 EDT
  std::cout.imbue(loc);
  std::cout << ny_time << std::endl;
  // 2004-Aug-30 10:00:00 Eastern Daylight Time

  return 0;
}


/*  Copyright 2004-2005: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */
