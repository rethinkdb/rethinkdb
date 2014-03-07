/* This example prints all the dates in a month. It demonstrates
 * the use of iterators as well as functions of the gregorian_calendar
 * 
 * Output:
 * Enter Year: 2002
 * Enter Month(1..12): 2
 * 2002-Feb-01 [Fri]
 * 2002-Feb-02 [Sat]
 * 2002-Feb-03 [Sun]
 * 2002-Feb-04 [Mon]
 * 2002-Feb-05 [Tue]
 * 2002-Feb-06 [Wed]
 * 2002-Feb-07 [Thu]
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>

int
main()
{
  std::cout << "Enter Year: ";
  int year, month;
  std::cin >> year;
  std::cout << "Enter Month(1..12): ";
  std::cin >> month;

  using namespace boost::gregorian;
  try {
    //Use the calendar to get the last day of the month
    int eom_day = gregorian_calendar::end_of_month_day(year,month);
    date endOfMonth(year,month,eom_day);

    //construct an iterator starting with firt day of the month
    day_iterator ditr(date(year,month,1));
    //loop thru the days and print each one
    for (; ditr <= endOfMonth; ++ditr) {
#if defined(BOOST_DATE_TIME_NO_LOCALE) 
      std::cout << to_simple_string(*ditr) << " ["
#else
      std::cout << *ditr << " ["
#endif
                << ditr->day_of_week() << " week: "
                << ditr->week_number() << "]"
                << std::endl; 
    }
  }
  catch(std::exception& e) {

    std::cout << "Error bad date, check your entry: \n"
              << "  Details: " << e.what() << std::endl;
  }
  return 0;
}

/*  Copyright 2001-2004: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

