
#include "boost/date_time/local_time/local_time.hpp"
#include <iostream>

/* This example shows a program that calculates the arrival time of a plane
 * that flys from Phoenix to New York.  During the flight New York shifts
 * into daylight savings time (Phoenix doesn't because Arizona doesn't use
 * DST).  
 *
 * 
 */

int main()
{
  using namespace boost::gregorian; 
  using namespace boost::local_time;
  using namespace boost::posix_time;


  //setup some timezones for creating and adjusting local times
  //This user editable file can be found in libs/date_time/data.
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
  time_zone_ptr nyc_tz = tz_db.time_zone_from_region("America/New_York");
  //Use a newly created time zone rule
  time_zone_ptr phx_tz(new posix_time_zone("MST-07:00:00"));

  //local departure time in Phoenix is 11 pm on March 13 2010 
  // (NY changes to DST on March 14 at 2 am)
  local_date_time phx_departure(date(2010, Mar, 13), hours(23), 
                                phx_tz, 
                                local_date_time::NOT_DATE_TIME_ON_ERROR);
  local_date_time nyc_departure = phx_departure.local_time_in(nyc_tz);

  time_duration flight_length = hours(4) + minutes(30);
  local_date_time phx_arrival = phx_departure + flight_length;
  local_date_time nyc_arrival = phx_arrival.local_time_in(nyc_tz);

  std::cout << "departure PHX time: " << phx_departure << std::endl;
  std::cout << "departure NYC time: " << nyc_departure << std::endl;
  std::cout << "arrival PHX time:   " << phx_arrival << std::endl;
  std::cout << "arrival NYC time:   " << nyc_arrival << std::endl;

}


/*  Copyright 2005: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */
