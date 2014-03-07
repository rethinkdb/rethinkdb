/* Boost test/fmod.cpp
 * test the fmod with specially crafted integer intervals
 *
 * Copyright 2002-2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval/interval.hpp>
#include <boost/numeric/interval/arith.hpp>
#include <boost/numeric/interval/arith2.hpp>
#include <boost/numeric/interval/utility.hpp>
#include <boost/numeric/interval/checking.hpp>
#include <boost/numeric/interval/rounding.hpp>
#include <boost/test/minimal.hpp>
#include "bugs.hpp"

struct my_rounded_arith {
  int sub_down(int x, int y) { return x - y; }
  int sub_up  (int x, int y) { return x - y; }
  int mul_down(int x, int y) { return x * y; }
  int mul_up  (int x, int y) { return x * y; }
  int div_down(int x, int y) {
    int q = x / y;
    return (x % y < 0) ? (q - 1) : q;
  }
  int int_down(int x) { return x; }
};

using namespace boost;
using namespace numeric;
using namespace interval_lib;

typedef change_rounding<interval<int>, save_state_nothing<my_rounded_arith> >::type I;

int test_main(int, char *[]) {

  BOOST_CHECK(equal(fmod(I(6,9), 7), I(6,9)));
  BOOST_CHECK(equal(fmod(6, I(7,8)), I(6,6)));
  BOOST_CHECK(equal(fmod(I(6,9), I(7,8)), I(6,9)));

  BOOST_CHECK(equal(fmod(I(13,17), 7), I(6,10)));
  BOOST_CHECK(equal(fmod(13, I(7,8)), I(5,6)));
  BOOST_CHECK(equal(fmod(I(13,17), I(7,8)), I(5,10)));

  BOOST_CHECK(equal(fmod(I(-17,-13), 7), I(4,8)));
  BOOST_CHECK(equal(fmod(-17, I(7,8)), I(4,7)));
  BOOST_CHECK(equal(fmod(I(-17,-13), I(7,8)), I(4,11)));

  return 0;
}
