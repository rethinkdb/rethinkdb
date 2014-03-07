/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include "boost/date_time/int_adapter.hpp"
#include "testfrmwk.hpp"
#include "boost/cstdint.hpp"
#include <iostream>
#include <sstream>

template<typename int_type> 
void print()
{
  //MSVC 6 has problems with this, but it's not really important 
  //so we will just skip them....
#if (defined(BOOST_DATE_TIME_NO_LOCALE)) || (defined(BOOST_MSVC) && (_MSC_VER < 1300))

#else
  std::cout << "min:       " << (int_type::min)().as_number() << std::endl;
  std::cout << "max:       " << (int_type::max)().as_number() << std::endl;
  std::cout << "neg_infin: " << 
    int_type::neg_infinity().as_number() << std::endl;
  std::cout << "pos_infin: " << 
    int_type::pos_infinity().as_number() << std::endl;
  std::cout << "not a number:  " << 
    int_type::not_a_number().as_number() << std::endl;
  std::stringstream ss("");
  std::string s("");
  int_type i = int_type::neg_infinity();
  ss << i;
  s = "-infinity";
  check("streaming -infinity", ss.str() == s);
  
  i = int_type::pos_infinity();
  ss.str("");
  ss << i;
  s = "+infinity";
  check("streaming +infinity", ss.str() == s);
  
  i = int_type::not_a_number();
  ss.str("");
  ss << i;
  s = "not-a-number";
  check("streaming nan", ss.str() == s);

  i = 12;
  ss.str("");
  ss << i;
  s = "12";
  check("streaming digits", ss.str() == s);
#endif
}


template<typename int_type> 
void test_int()
{
  int_type i = int_type::neg_infinity();

  check("is infinity", i.is_infinity());
  check("is special_value (neg_inf)", i.is_special());
  check("as_special convert", boost::date_time::neg_infin == i.as_special() );
  check("as_special convert", boost::date_time::neg_infin == int_type::to_special(i.as_number()) );
  int_type h = int_type::neg_infinity();
  i = int_type::pos_infinity();
  check("is infinity", i.is_infinity());
  check("as_special convert", boost::date_time::pos_infin == i.as_special() );
  check("infinity less",     h < i);
  check("infinity less",     h < 0);
  check("infinity greater",  i > h);
  check("infinity greater",  i > 0);
  h = int_type::not_a_number();
  check("nan less",  !(h < 0));
  check("nan greater",  !(h > 0));
  check("nan equal",  h == int_type::not_a_number());
  i = 1;
  check("is infinity", !i.is_infinity());
  int_type j = int_type::neg_infinity();
  check("infinity less",     j < i);
  check("infinity less",     !(j < j));
  check("infinity greater",  (i > j));
  check("infinity equal",    !(j == i));
  check("infinity equal",    j == j);
  check("infinity equal",    !(j == 0));
  check("infinity not equal",    j != 0);

  int_type k = 1;
  check("as_special convert", boost::date_time::not_special == k.as_special() );
  check("equal",             i == k);
  check("infinity not equal",    i != int_type::neg_infinity());
  check("infinity not equal",    i != int_type::pos_infinity());
  int_type l = i + int_type::pos_infinity();
  check("is special_value (pos_inf)", l.is_special());
  check("add infinity" ,         l == int_type::pos_infinity());
  { // limiting the scope for these tests was easier than recalculating l
    int_type l = i - int_type::pos_infinity();
    check("value - +infinity",         l == int_type::neg_infinity());
    l = i + int_type::neg_infinity();
    check("value + -infinity",         l == int_type::neg_infinity());
  }
  check("inf - inf = nan",       (l - l) == int_type::not_a_number());
  check("-inf + inf = nan",       (j + l) == int_type::not_a_number());
  check("add 2",                 (i + 2) == 3);
  i = int_type::not_a_number();
  check("+inf * integer", (l * 2) == l);
  check("+inf / integer", (l / 2) == l);
  check("+inf % -integer", (l % -2) == j);
  check("+inf % integer", (l % 2) == l);
  check("+inf / -integer", (l / -2) == j);
  check("+inf * -integer", (l * -2) == j);
  check("+inf * -inf", (l * j) == j);
  check("+inf / +inf", (l / l) == i);
  check("+inf % +inf", (l % l) == i);
  check("+inf * zero", (l * 0) == i);
  check("is special_value (nan)", i.is_special());
  check("as_special convert", boost::date_time::not_a_date_time == i.as_special() );
  check("add not a number",      (i + 2) == int_type::not_a_number());
  check("sub not a number",      (i - 2) == int_type::not_a_number());
  check("sub from infin",        (l - 2) == int_type::pos_infinity());
  i = 5;
  h = 3;
  check("add zero ",             (i + 0) == 5);
  check("sub from 5-2 ",         (i - 2) == 3);
  check("remainder from 5/2 ",   (i % 2) == 1);
  check("remainder from 5/3 ",   (i % h) == 2);
  //  std::cout << i.as_number() << std::endl;
  check("from special ", 
        int_type::from_special(boost::date_time::pos_infin) == int_type::pos_infinity());
  check("from special ", 
        int_type::from_special(boost::date_time::neg_infin) == int_type::neg_infinity());
  check("from special ", 
        int_type::from_special(boost::date_time::not_a_date_time) == int_type::not_a_number());
  check("from special ", 
        int_type::from_special(boost::date_time::min_date_time) == (int_type::min)());
  check("from special ", 
        int_type::from_special(boost::date_time::max_date_time) == (int_type::max)());
}

int
main() 
{
  using namespace boost::date_time;

  print< int_adapter<unsigned long> >();
  test_int< int_adapter<unsigned long> >();
  print< int_adapter<long> >();
  test_int< int_adapter<long> >();
  print< int_adapter<boost::int64_t> >();
  test_int< int_adapter<boost::int64_t> >();


  return printTestStats();

}

