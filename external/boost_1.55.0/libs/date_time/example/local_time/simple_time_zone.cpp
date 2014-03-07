/* A simple example for using a custom_time_zone and a posix_time_zone.
 */

#include "boost/date_time/local_time/local_time.hpp"
#include <iostream>

int
main() 
{
  using namespace boost;
  using namespace local_time;
  using namespace gregorian;
  using posix_time::time_duration;

  /***** custom_time_zone *****/
  
  // create the dependent objects for a custom_time_zone
  time_zone_names tzn("Eastern Standard Time", "EST",
                      "Eastern Daylight Time", "EDT");
  time_duration utc_offset(-5,0,0);
  dst_adjustment_offsets adj_offsets(time_duration(1,0,0), 
                                     time_duration(2,0,0), 
                                     time_duration(2,0,0));
  // rules for this zone are:
  // start on first Sunday of April at 2 am
  // end on last Sunday of October at 2 am
  // so we use a first_last_dst_rule
  first_day_of_the_week_in_month start_rule(Sunday, Apr);
  last_day_of_the_week_in_month    end_rule(Sunday, Oct);
  shared_ptr<dst_calc_rule> nyc_rules(new first_last_dst_rule(start_rule, 
                                                              end_rule));
  // create more dependent objects for a non-dst custom_time_zone
  time_zone_names tzn2("Mountain Standard Time", "MST",
                       "", ""); // no dst means empty dst strings
  time_duration utc_offset2(-7,0,0);
  dst_adjustment_offsets adj_offsets2(time_duration(0,0,0), 
                                      time_duration(0,0,0), 
                                      time_duration(0,0,0));
  // no dst means we need a null pointer to the rules
  shared_ptr<dst_calc_rule> phx_rules;

  // create the custom_time_zones
  time_zone_ptr nyc_1(new custom_time_zone(tzn, utc_offset, adj_offsets, nyc_rules));
  time_zone_ptr phx_1(new custom_time_zone(tzn2, utc_offset2, adj_offsets2, phx_rules));

  /***** posix_time_zone *****/

  // create posix_time_zones that are the duplicates of the 
  // custom_time_zones created above. See posix_time_zone documentation 
  // for details on full zone names.
  std::string nyc_string, phx_string;
  nyc_string = "EST-05:00:00EDT+01:00:00,M4.1.0/02:00:00,M10.5.0/02:00:00";
  // nyc_string = "EST-05EDT,M4.1.0,M10.5.0"; // shorter when defaults used
  phx_string = "MST-07"; // no-dst
  time_zone_ptr nyc_2(new posix_time_zone(nyc_string));
  time_zone_ptr phx_2(new posix_time_zone(phx_string));
 

  /***** show the sets are equal *****/

  std::cout << "The first zone is in daylight savings from:\n " 
    << nyc_1->dst_local_start_time(2004) << " through "
    << nyc_1->dst_local_end_time(2004) << std::endl;

  std::cout << "The second zone is in daylight savings from:\n " 
    << nyc_2->dst_local_start_time(2004) << " through "
    << nyc_2->dst_local_end_time(2004) << std::endl;

  std::cout << "The third zone (no daylight savings):\n " 
    << phx_1->std_zone_abbrev() << " and "
    << phx_1->base_utc_offset() << std::endl;

  std::cout << "The fourth zone (no daylight savings):\n " 
    << phx_2->std_zone_abbrev() << " and "
    << phx_2->base_utc_offset() << std::endl;

  return 0;
}

/*  Copyright 2001-2005: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

