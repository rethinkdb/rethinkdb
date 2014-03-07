/* Boost test/pow.cpp
 * test the pow function
 *
 * Copyright 2002-2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <boost/test/minimal.hpp>
#include "bugs.hpp"

bool test_pow(double al, double au, double bl, double bu, int p) {
  typedef boost::numeric::interval<double> I;
  I b = pow(I(al, au), p);
  return b.lower() == bl && b.upper() == bu;
}

int test_main(int, char *[]) {
  BOOST_CHECK(test_pow(2, 3, 8, 27, 3));
  BOOST_CHECK(test_pow(2, 3, 16, 81, 4));
  BOOST_CHECK(test_pow(-3, 2, -27, 8, 3));
  BOOST_CHECK(test_pow(-3, 2, 0, 81, 4));
  BOOST_CHECK(test_pow(-3, -2, -27, -8, 3));
  BOOST_CHECK(test_pow(-3, -2, 16, 81, 4));

  BOOST_CHECK(test_pow(2, 4, 1./64, 1./8, -3));
  BOOST_CHECK(test_pow(2, 4, 1./256, 1./16, -4));
  BOOST_CHECK(test_pow(-4, -2, -1./8, -1./64, -3));
  BOOST_CHECK(test_pow(-4, -2, 1./256, 1./16, -4));

  BOOST_CHECK(test_pow(2, 3, 1, 1, 0));
  BOOST_CHECK(test_pow(-3, 2, 1, 1, 0));
  BOOST_CHECK(test_pow(-3, -2, 1, 1, 0));

# ifdef __BORLANDC__
  ::detail::ignore_warnings();
# endif
  return 0;
}
