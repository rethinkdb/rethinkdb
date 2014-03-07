/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"


int main(){

#if !defined(BOOST_DATE_TIME_OPTIONAL_GREGORIAN_TYPES)
  // do not set this test to return fail - 
  // this is not necessarily a compiler problem
  check("Optional gregorian types not selected - no tests run", true);
#else

  using namespace boost::gregorian;
      

  /*** months ***/
  {
    months m1(5), m2(3), m3(1);
    check("months & months addable", months(8) == m1 + m2);
    m1 += m2;
    check("months & months addable", months(8) == m1);
    check("months & months subtractable", months(-5) == m2 - m1);
    m2 -= m1;
    check("months & months subtractable", months(-5) == m2);
    {
      // adding and subtracting negative values
      date d1(2005, Jan, 1);
      date d2(2005, Feb, 1);
      check("add neg months (year wrap under)",
          d1 + months(-1) == date(2004,Dec,1));
      check("add neg months (no year wrap under)",
          d2 + months(-1) == date(2005,Jan,1));
      check("add neg months (year wrap under)",
          d2 + months(-2) == date(2004,Dec,1));
      check("add neg months (year wrap under)",
          d2 + months(-12) == date(2004,Feb,1));
      check("add neg months (year wrap under)",
          d2 + months(-13) == date(2004,Jan,1));
      check("add neg months (year wrap under)",
          d2 + months(-14) == date(2003,Dec,1));
      date d3(2005, Dec, 1);
      date d4(2005, Nov, 1);
      check("subtract neg months (year wrap over)",
          d3 - months(-1) == date(2006,Jan,1));
      check("subtract neg months (no year wrap over)",
          d4 - months(-1) == date(2005,Dec,1));
      check("subtract neg months (year wrap over)",
          d4 - months(-2) == date(2006,Jan,1));
      check("subtract neg months (year wrap over)",
          d4 - months(-12) == date(2006,Nov,1));
      check("subtract neg months (year wrap over)",
          d4 - months(-13) == date(2006,Dec,1));
      check("subtract neg months (year wrap over)",
          d4 - months(-14) == date(2007,Jan,1));
    }
    {
      months m1(5), m2(3), m3(10);
      check("months & int multipliable", months(15) == m1 * 3);
      m1 *= 3;
      check("months & int multipliable", months(15) == m1);
      //check("int * months", months(12) == 4 * m2);
      check("months & int dividable", months(3) == m3 / 3);
      m3 /= 3;
      check("months & int dividable", months(3) == m3);
    }
    {
      months m(-5), m_pos(pos_infin), m_neg(neg_infin), m_nadt(not_a_date_time);
      check("months add special_values", m + m_pos == m_pos);
      check("months add special_values", m + m_neg == m_neg);
      check("months add special_values", m_pos + m_neg == m_nadt);
      check("months add special_values", m_neg + m_neg == m_neg);
      check("months subtract special_values", m - m_pos == m_neg);
      check("months subtract special_values", m - m_neg == m_pos);
      check("months subtract special_values", m_pos - m_neg == m_pos);
      check("months special_values & int multipliable", m_pos * -1 == m_neg);
      check("months special_values & int multipliable", m_pos * 0 == m_nadt);
      check("months special_values & int dividable", m_neg / 3 == m_neg);
    }

    years y1(2), y2(4);
    check("months & years addable", months(25) == m3 + y1);
    m3 += y1;
    check("months & years addable", months(25) == m3);
    check("months & years subtractable", months(-23) == m3 - y2);
    m3 -= y2;
    check("months & years subtractable", months(-23) == m3);

    {
      date d(2001, Oct, 31);
      check("date + months", date(2002, Feb, 28) == d + months(4));
      d += months(4);
      check("date += months", date(2002, Feb, 28) == d);
    }
    {
      date d(2001, Oct, 31);
      check("date - months", date(2001, Apr, 30) == d - months(6));
      d -= months(6);
      check("date -= months", date(2001, Apr, 30) == d);
    }
  }

  /*** years ***/
  {
    years y1(2), y2(4), y3(1);
    check("years & years addable", years(3) == y3 + y1);
    y3 += y1;
    check("years & years addable", years(3) == y3);
    check("years & years subtractable", years(-1) == y3 - y2);
    y3 -= y2;
    check("years & years subtractable", years(-1) == y3);
    {
      years y1(5), y2(3), y3(10);
      check("years & int multipliable", years(15) == y1 * 3);
      y1 *= 3;
      check("years & int multipliable", years(15) == y1);
      //check("int * years", years(12) == 4 * y2);
      check("years & int dividable", years(3) == y3 / 3);
      y3 /= 3;
      check("years & int dividable", years(3) == y3);
    }
    {
      years m(15), y_pos(pos_infin), y_neg(neg_infin), y_nadt(not_a_date_time);
      check("years add special_values", m + y_pos == y_pos);
      check("years add special_values", m + y_neg == y_neg);
      check("years add special_values", y_pos + y_neg == y_nadt);
      check("years add special_values", y_neg + y_neg == y_neg);
      check("years subtract special_values", m - y_pos == y_neg);
      check("years subtract special_values", m - y_neg == y_pos);
      check("years subtract special_values", y_pos - y_neg == y_pos);
      check("years special_values & int multipliable", y_pos * -1 == y_neg);
      check("years special_values & int multipliable", y_pos * 0 == y_nadt);
      check("years special_values & int dividable", y_neg / 3 == y_neg);
    }
    
    months m1(5), m2(3);
    check("years & months addable", months(51) == y2 + m2);
    check("years & months subtractable", months(43) == y2 - m1);

    {
      date d(2001, Feb, 28); // not a leap year
      check("date + years", date(2004, Feb, 29) == d + years(3));
      d += years(3);
      check("date += years", date(2004, Feb, 29) == d);
    }
    {
      date d(2000, Feb, 29);
      check("date - years", date(1994, Feb, 28) == d - years(6));
      d -= years(6);
      check("date -= years", date(1994, Feb, 28) == d);
    }

  }
  
  /*** weeks ***/
  // shouldn't need many tests, it is nothing more than a date_duration
  // so all date_duration tests should prove this class
  {
    weeks w1(2), w2(4), w3(1), pi(pos_infin);
    check("add special_values", weeks(pos_infin) == w1 + pi);
    check("weeks & weeks addable", weeks(3) == w3 + w1);
    w3 += w1;
    check("weeks & weeks addable", weeks(3) == w3);
    check("weeks & weeks subtractable", weeks(-1) == w3 - w2);
    w3 -= w2;
    check("weeks & weeks subtractable", weeks(-1) == w3);
    {
      days d(10);
      check("days + weeks", days(31) == d + weeks(3));
      d += weeks(3);
      check("days += weeks", days(31) == d);
    }
    {
      days d(10);
      check("days - weeks", days(-32) == d - weeks(6));
      d -= weeks(6);
      check("days -= weeks", days(-32) == d);
    }
    {
      date d(2001, Feb, 28);
      check("date + weeks", date(2001, Mar, 21) == d + weeks(3));
      d += weeks(3);
      check("date += weeks", date(2001, Mar, 21) == d);
    }
    {
      date d(2001, Feb, 28);
      check("date - weeks", date(2001, Jan, 17) == d - weeks(6));
      d -= weeks(6);
      check("date -= weeks", date(2001, Jan, 17) == d);
    }
  }

  {
    date d(2000, Oct, 31);
    date d2 = d + months(4) + years(2);
    date d3 = d + years(2) + months(4);
    check("date + years + months", date(2003,Feb,28) == d2);
    check("date + years + months", date(2003,Feb,28) == d3);
    months m = years(2) + months(4) - months(4) - years(2);
    check("sanity check", m.number_of_months() == 0);
  }
  /*{
    date d(2001, Mar, 31);
    date d1 = (d - months(1)) + months(1); //Mar 28, right? WRONG
    // Mar31 - 1 month is Feb28 (last day of month) so Feb28 + 1 month
    // will be Mar31 (last day of month)
    check("date + 1 months - 1 months", date(2001,Mar,28) == d1);
    std::cout << d1 << std::endl;
    //date d2 = (d - months(1)) + d; //compile error, right?  RIGHT 
    //weeks w1 = weeks(1) + months(1); //compiler error, right?  RIGHT 
  }*/

#endif // BOOST_DATE_TIME_OPTIONAL_GREGORIAN_TYPES
  
  return printTestStats();
  
}
