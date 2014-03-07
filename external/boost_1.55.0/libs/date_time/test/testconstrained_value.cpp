/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/config.hpp"
#include "boost/date_time/constrained_value.hpp"
#include "testfrmwk.hpp"
#include <iostream>

class bad_day {}; //exception type


class day_value_policies
{
public:
  typedef unsigned int value_type;
  static unsigned int min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; };
  static unsigned int max BOOST_PREVENT_MACRO_SUBSTITUTION () { return 31;};
  static void on_error(unsigned int&, unsigned int, boost::CV::violation_enum)
  {
    throw bad_day();
  }
};

struct range_error {}; //exception type
typedef boost::CV::simple_exception_policy<int,1,10,range_error> one_to_ten_range_policy;

int main()
{
  using namespace boost::CV;
  constrained_value<day_value_policies> cv1(0), cv2(31);
  check("not equal", cv1 != cv2);
  check("equal", cv1 == cv1);
  check("greater", cv2 > cv1);
  check("greater or equal ", cv2 >= cv1);
  //various running of the conversion operator
  std::cout << cv1 << std::endl;
  unsigned int i = cv1; 
  check("test conversion", i == cv1);


  try {
    constrained_value<one_to_ten_range_policy> cv2(11);
    std::cout << "Not Reachable: " << cv2 << " ";
    check("got range exception max", false);
  }
  catch(range_error& e) {
    e = e; // removes compiler warning
    check("got range exception max", true);
  }

  try {
    constrained_value<one_to_ten_range_policy> cv3(0);
    std::cout << "Not Reachable: " << cv3 << " ";
    check("got range exception min", false);
  }
  catch(range_error& e) {
    e = e; // removes compiler warning
    check("got range exception min", true);
  }

  try {
    constrained_value<one_to_ten_range_policy> cv4(1);
    cv4 = 12;
    check("range exception on assign", false);
  }
  catch(range_error& e) {
    e = e; // removes compiler warning
    check("range exception on assign", true);
  }

  return printTestStats();
}

