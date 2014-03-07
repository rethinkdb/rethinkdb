/*
This example demonstrates a simple use of periods for the calculation
of date information.

The example calculates if a given date is a weekend or holiday
given an exclusion set.  That is, each weekend or holiday is
entered into the set as a time interval.  Then if a given date
is contained within any of the intervals it is considered to
be within the exclusion set and hence is a offtime.

Output:
Number Excluded Periods: 5
20020202/20020203
20020209/20020210
20020212/20020212
20020216/20020217
In Exclusion Period: 20020216 --> 20020216/20020217
20020223/20020224

*/


#include "boost/date_time/gregorian/gregorian.hpp"
#include <set>
#include <algorithm>
#include <iostream>

typedef std::set<boost::gregorian::date_period> date_period_set;

//Simple population of the exclusion set
date_period_set
generateExclusion()
{
  using namespace boost::gregorian;
  date_period periods_array[] = 
    { date_period(date(2002,Feb,2), date(2002,Feb,4)),//weekend of 2nd-3rd
      date_period(date(2002,Feb,9), date(2002,Feb,11)),
      date_period(date(2002,Feb,16), date(2002,Feb,18)),
      date_period(date(2002,Feb,23), date(2002,Feb,25)),
      date_period(date(2002,Feb,12), date(2002,Feb,13))//a random holiday 2-12
    };
  const int num_periods = sizeof(periods_array)/sizeof(date_period);

  date_period_set ps;
  //insert the periods in the set
  std::insert_iterator<date_period_set> itr(ps, ps.begin());
  std::copy(periods_array, periods_array+num_periods, itr );
  return ps;
  
}


int main() 
{
  using namespace boost::gregorian;
  
  date_period_set ps = generateExclusion();
  std::cout << "Number Excluded Periods: "  << ps.size() << std::endl;

  date d(2002,Feb,16);
  date_period_set::const_iterator i = ps.begin();
  //print the periods, check for containment
  for (;i != ps.end(); i++) {
    std::cout << to_iso_string(*i) << std::endl;
    //if date is in exclusion period then print it
    if (i->contains(d)) {
      std::cout << "In Exclusion Period: "
                << to_iso_string(d) << " --> " << to_iso_string(*i)
                << std::endl;
    }
  }

  return 0;  

}




/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

