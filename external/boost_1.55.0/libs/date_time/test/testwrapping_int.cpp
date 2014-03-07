/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst 
 */

#include "boost/date_time/wrapping_int.hpp"
//#define BOOST_INCLUDE_MAIN
//#include <boost/test/test_tools.hpp>
#include "testfrmwk.hpp"
#include "boost/cstdint.hpp"
#include <iostream>


int
main() 
// int 
// test_main(int, char*[])
{
  using namespace boost::date_time;
  
  wrapping_int<int, 3600> wi(3599);
  check("construction/conversion", wi == 3599);
  check("add with wrap",    wi.add(1) == 1);
  check("added value ok",   wi == 0);
  check("add with 2 wraps", wi.add(7201) == 2);
  check("added value ok",   wi == 1);
  check("add with 3 wraps", wi.add(10800) == 3);
  check("added value ok",   wi == 1);
  check("subtract no wrap", wi.subtract(1) == 0);
  check("subtract val ok",  wi == 0);
  check("subtract no wrap", wi.subtract(3601) == 2);
  check("subtract val ok",  wi == 3599);
  check("add again",        (wi.add(2) == 1) && (wi == 1));
  check("subtract again",   (wi.subtract(2) == 1) && (wi == 3599));
  check("add again",        (wi.add(2) == 1) && (wi == 1));
  check("subtract again",   (wi.subtract(3600) == 1) && (wi == 1));
  check("subtract again",   (wi.subtract(3599) == 1) && (wi == 2));
  check("subtract again",   (wi.subtract(1) == 0) && (wi == 1));
  std::cout << wi << std::endl;

  wrapping_int<short, 60> wi2(0);
  check("add with wrap - return",  wi2.add(121) == 2);
  check("add with wrap - value",   wi2 == 1);

  wrapping_int<short, 60> wi3(-5);
  check("signed int - add return",  wi3.add(5) == 0);
  check("signed int - value",   wi3 == 0);

  { // subtracting negative values
    wrapping_int<short, 10> wi3(5);
    check("subtract negative value to cause wrap",
          (wi3.subtract(-8) == -1 && wi3 == 3));
    check("reset", wi3.add(2) == 0 && wi3 ==5);
    check("add negative value to cause wrap",
          (wi3.add(-8) == -1 && wi3 == 7));
  }

  wrapping_int2<short, 1, 5> wi4(1);
  check("construct",  wi4 == 1);
  check("add up to wrap value",   (wi4.add(4) == 0 && wi4 == 5));
  check("add over the wrap value", (wi4.add(1) == 1 && wi4 == 1));
  check("add over the wrap value X 2", (wi4.add(10) == 2 && wi4 == 1));
  check("add over the wrap value X 3", (wi4.add(15) == 3 && wi4 == 1));

  wrapping_int2<short, 1, 12> wi5(12);
  check("construct",  wi5 == 12);
  check("add over the wrap value", (wi5.add(1) == 1 && wi5 == 1));
  
  check("subtract of the wrap value", (wi5.subtract(1) == -1 && wi5 == 12));
  check("subtract of the wrap value", (wi5.subtract(13) == -1 && wi5 == 11));

  // min_values other than 1
  wrapping_int2<short, 2, 6> wi6(2);
  check("construct",  wi6 == 2);
  check("add up to wrap value",   (wi6.add(4) == 0 && wi6 == 6));
  check("add over the wrap value", (wi6.add(1) == 1 && wi6 == 2));
  check("add over the wrap value X 2", wi6.add(11) == 2);
  check("add over the wrap value X 2", wi6 == 3);
  check("sub down to wrap value", wi6.subtract(1) == 0 && wi6 == 2);
  check("sub under the wrap value", wi6.subtract(1) == -1 && wi6 == 6);
  check("sub under the wrap value X 2", wi6.subtract(11) == -2 && wi6 == 5);
  //std::cout << wi6 << std::endl;

  // adding & subtracting negative values
  wrapping_int2<short, 1, 12> wi7(6);
  wrapping_int2<short, -5, 5> wi8(0);
  check("add negative value", (wi7.add(-2) == 0 && wi7 == 4));
  check("add negative value", (wi8.add(-2) == 0 && wi8 == -2));
  check("add negative value to cause single wrap", 
        (wi7.add(-6) == -1 && wi7 == 10));
  check("add negative value to cause single wrap", 
        (wi8.add(-5) == -1 && wi8 == 4));
  check("add negative value to cause multiple wrap", 
        (wi7.add(-22) == -2 && wi7 == 12));
  check("add negative value to cause multiple wrap", 
        (wi8.add(-22) == -2 && wi8 == 4));
  // reset values to mid range
  wi7.subtract(6);
  check("reset", wi7 == 6);
  wi8.subtract(4);
  check("reset", wi8 == 0);
  check("subtract negative value", (wi7.subtract(-2) == 0 && wi7 == 8));
  check("subtract negative value", (wi8.subtract(-2) == 0 && wi8 == 2));
  check("subtract negative value to cause single wrap", 
        (wi7.subtract(-6) == 1 && wi7 == 2));
  check("subtract negative value to cause single wrap", 
        (wi8.subtract(-5) == 1 && wi8 == -4));
  check("subtract negative value to cause multiple wrap", 
        (wi7.subtract(-23) == 2 && wi7 == 1));
  check("subtract negative value to cause multiple wrap", 
        (wi8.subtract(-22) == 2 && wi8 == -4));

// #ifdef BOOST_HAS_LONG_LONG
//   wrapping_int<boost::int64_t, 86400*100000LL> wi4(0);
//   check("construction/conversion", wi4 == 0);
//   boost::int64_t off2 = 372300000;
//   boost::int64_t wrap = 86400LL*100000LL;
//   boost::int64_t wrap2 = 86400000000;
//   wrapping_int<boost::int64_t,86400000000LL> wi5((3600*1 + 60*2 + 3)*100000);
//   std::cout << wi5 << std::endl;
//   boost::int64_t over = wi4.add(off2);
//   std::cout << over << std::endl;
//   std::cout << wrap << std::endl;
//   std::cout << wrap2 << std::endl;
//   //  check("construction/conversion", wi4 == 0);
// #endif

//   wrapping_int<int, 60> wi(121);
//   check("construction/conversion", wi == 121);
//   check("add with wrap",    wi.add(1) == 1);

  return printTestStats();

}


