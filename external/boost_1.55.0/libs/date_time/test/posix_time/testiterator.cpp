/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 2008-11-26 13:07:14 -0800 (Wed, 26 Nov 2008) $
 */

#include <iostream>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "../testfrmwk.hpp"
#if defined(BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS)
#include "boost/date_time/gregorian/formatters_limited.hpp"
#else
#include "boost/date_time/gregorian/formatters.hpp"
#endif

void iterate_backward(const boost::posix_time::ptime *answers, int ary_len, 
    const boost::posix_time::time_duration& td)
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  
  int i = ary_len -1;
  ptime end = answers[i];
  time_iterator titr(end,td);
  
  std::cout << "counting down by previous duration..." << std::endl;
  for (; titr >= answers[0]; --titr) {
    std::cout << to_simple_string(*titr) << std::endl;
    check("iterating backward", answers[i] == *titr);
    --i;
  }
  check("iterating backward count", i == -1); // check the number of iterations
  std::cout << std::endl;
}

int
main() 
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  date d(2000,Jan,20);
  ptime start(d);
  const ptime answer1[] = {ptime(d), ptime(d,seconds(1)),
                           ptime(d,seconds(2)), ptime(d,seconds(3))};
  int i=0;
  time_iterator titr(start,seconds(1)); 
  for (; titr < ptime(d,seconds(4)); ++titr) {
    std::cout << to_simple_string(*titr) << std::endl;
    check("iterator -- 1 sec", answer1[i] == *titr);
    i++;
  }
  check("iterator -- 1 sec", i == 4); // check the number of iterations

  iterate_backward(answer1, 4, seconds(1));

  //iterate by hours
  const ptime answer2[] = {ptime(d), ptime(d,hours(1)),
                           ptime(d,hours(2)), ptime(d,hours(3))};
  i=0;
  time_iterator titr2(start,hours(1)); 
  for (; titr2 < ptime(d,hours(4)); ++titr2) {
    std::cout << to_simple_string(*titr2) << std::endl;
    check("iterator -- 1 hour", answer2[i] == *titr2);
    i++;
  }
  check("iterator -- 1 hour", i == 4); // check the number of iterations

  iterate_backward(answer2, 4, hours(1));


  //iterate by 15 mintues
  const ptime answer3[] = {ptime(d), ptime(d,minutes(15)),
                           ptime(d,minutes(30)), ptime(d,minutes(45)),
                           ptime(d,minutes(60)), ptime(d,minutes(75))};
  i=0;
  time_iterator titr3(start,minutes(15)); 
  for (; titr3 < ptime(d,time_duration(1,20,0)); ++titr3) {
    std::cout << to_simple_string(*titr3) << std::endl;
    check("iterator -- 15 min", answer3[i] == *titr3);
    i++;
  }
  check("iterator -- 15 min", i == 6); // check the number of iterations

  iterate_backward(answer3, 6, minutes(15));

  //iterate by .1 seconds
  const ptime answer4[] = {ptime(d), ptime(d,time_duration(0,0,0,1000)),
                           ptime(d,time_duration(0,0,0,2000)),
                           ptime(d,time_duration(0,0,0,3000))};
  i=0;
  time_iterator titr4(start,time_duration(0,0,0,1000)); 
  for (; titr4 < ptime(d,time_duration(0,0,0,4000)); ++titr4) {
    std::cout << to_simple_string(*titr4) << std::endl;
    check("iterator -- tenth sec", answer4[i] == *titr4);
    i++;
  }
  check("iterator -- tenth sec", i == 4); // check the number of iterations

  iterate_backward(answer4, 4, time_duration(0,0,0,1000));

  //iterate by crazy duration
  time_duration crzyd = duration_from_string("2:18:32.423"); 
  const ptime answer5[] = {ptime(d), ptime(d,crzyd),
                           ptime(d, crzyd * 2),
                           ptime(d, crzyd * 3)};
  i=0;
  time_iterator titr5(start,crzyd); 
  for (; titr5 < ptime(d,crzyd * 4); ++titr5) {
    std::cout << to_simple_string(*titr5) << std::endl;
    check("iterator -- crazy duration", answer5[i] == *titr5);
    i++;
  }
  check("iterator -- crazy duration", i == 4); // check the number of iterations

  iterate_backward(answer5, 4, crzyd);

  //iterate up by neg_dur
  time_duration pos_dur = minutes(3);
  time_duration neg_dur = -pos_dur;
  time_iterator up_itr(start, neg_dur), dn_itr(start, pos_dur);

  for(i=0; i < 10; ++i)
  {
    check("up-by-neg == backward iterate", *up_itr == *dn_itr);
    ++up_itr; // up by -3
    --dn_itr; // down by 3
  }
  
 
  return printTestStats();
}

