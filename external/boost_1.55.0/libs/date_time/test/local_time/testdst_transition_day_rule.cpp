/* Copyright (c) 2003-2004 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/local_time/dst_transition_day_rules.hpp"
#include "boost/shared_ptr.hpp"
#include "../testfrmwk.hpp"



// see http://www.timeanddate.com/time/aboutdst.html for some info
// also 
int
main() 
{
  //  using namespace boost::posix_time;
  using namespace boost::local_time;
  using namespace boost::gregorian;

  boost::shared_ptr<dst_calc_rule> 
    rule1(new partial_date_dst_rule(partial_date(30,Apr),
                                    partial_date(30,Oct)));

  check("partial date rule", rule1->start_day(2001) == date(2001, Apr, 30));
  check("partial date rule", rule1->end_day(2001) == date(2001, Oct, 30));
    
  boost::shared_ptr<dst_calc_rule> 
    rule2(new first_last_dst_rule(first_last_dst_rule::start_rule(Sunday,Apr),
                                  first_last_dst_rule::end_rule(Sunday,Oct)));

  check("first last rule", rule2->start_day(2001) == date(2001, Apr, 1));
  check("first last rule", rule2->end_day(2001) == date(2001, Oct, 28));

  boost::shared_ptr<dst_calc_rule> 
    rule3(new last_last_dst_rule(last_last_dst_rule::start_rule(Sunday,Mar),
                                 last_last_dst_rule::end_rule(Sunday,Oct)));

  check("last last rule", rule3->start_day(2001) == date(2001, Mar, 25));
  check("last last rule", rule3->end_day(2001) == date(2001, Oct, 28));
  
  typedef nth_kday_of_month nkday;
  boost::shared_ptr<dst_calc_rule> 
    rule4(new nth_last_dst_rule(nth_last_dst_rule::start_rule(nkday::first,Sunday,Mar),
                                nth_last_dst_rule::end_rule(Sunday,Oct)));
  
  check("nth Last rule", rule4->start_day(2001) == date(2001, Mar, 4));
  check("nth Last rule", rule4->end_day(2001) == date(2001, Oct, 28));
    
  boost::shared_ptr<dst_calc_rule> 
    rule5(new nth_kday_dst_rule(nth_kday_dst_rule::start_rule(nkday::first,Sunday,Mar),
                                nth_kday_dst_rule::end_rule(nkday::fourth,Sunday,Oct)));
  
  check("nth_kday rule", rule5->start_day(2001) == date(2001, Mar, 4));
  check("nth_kday rule", rule5->end_day(2001) == date(2001, Oct, 28));
  


  return printTestStats();

}

