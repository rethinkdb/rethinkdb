/* A simple example for creating various dst_calc_rule instances
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/local_time/local_time.hpp"
#include <iostream>

int
main() 
{
  using namespace boost;
  using namespace local_time;
  using namespace gregorian;

  /***** create the necessary date_generator objects *****/
  // starting generators
  first_day_of_the_week_in_month fd_start(Sunday, May);
  last_day_of_the_week_in_month ld_start(Sunday, May);
  nth_day_of_the_week_in_month nkd_start(nth_day_of_the_week_in_month::third, 
                                         Sunday, May);
  partial_date pd_start(1, May);
  // ending generators
  first_day_of_the_week_in_month fd_end(Sunday, Oct);
  last_day_of_the_week_in_month ld_end(Sunday, Oct);
  nth_day_of_the_week_in_month nkd_end(nth_day_of_the_week_in_month::third, 
                                       Sunday, Oct);
  partial_date pd_end(31, Oct);

  /***** create the various dst_calc_rule objects *****/
  dst_calc_rule_ptr pdr(new partial_date_dst_rule(pd_start, pd_end));
  dst_calc_rule_ptr flr(new first_last_dst_rule(fd_start, ld_end));
  dst_calc_rule_ptr llr(new last_last_dst_rule(ld_start, ld_end));
  dst_calc_rule_ptr nlr(new nth_last_dst_rule(nkd_start, ld_end));
  dst_calc_rule_ptr ndr(new nth_day_of_the_week_in_month_dst_rule(nkd_start, nkd_end));

  std::cout << "Program run successfully" << std::endl;

  return 0;
}

/*  Copyright 2001-2005: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

