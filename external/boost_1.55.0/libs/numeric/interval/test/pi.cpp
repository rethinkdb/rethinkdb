/* Boost test/pi.cpp
 * test if the pi constant is correctly defined
 *
 * Copyright 2002-2003 Guillaume Melquiond, Sylvain Pion
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <boost/limits.hpp>
#include <boost/test/minimal.hpp>
#include "bugs.hpp"

#define PI 3.14159265358979323846

typedef boost::numeric::interval<int>         I_i;
typedef boost::numeric::interval<float>       I_f;
typedef boost::numeric::interval<double>      I_d;
typedef boost::numeric::interval<long double> I_ld;

using boost::numeric::interval_lib::pi;
using boost::numeric::interval_lib::pi_half;
using boost::numeric::interval_lib::pi_twice;

int test_main(int, char *[]) {
  I_i  pi_i  = pi<I_i>();
  I_f  pi_f  = pi<I_f>();
  I_d  pi_d  = pi<I_d>();
  I_ld pi_ld = pi<I_ld>();

  BOOST_CHECK(in((int)   PI, pi_i));
  BOOST_CHECK(in((float) PI, pi_f));
  BOOST_CHECK(in((double)PI, pi_d));
  BOOST_CHECK(subset(pi_i, widen(I_i((int)   PI), 1)));
  BOOST_CHECK(subset(pi_f, widen(I_f((float) PI), (std::numeric_limits<float> ::min)())));
  BOOST_CHECK(subset(pi_d, widen(I_d((double)PI), (std::numeric_limits<double>::min)())));

  // We can't test the following equalities for interval<int>.
  I_f pi_f_half = pi_half<I_f>();
  I_f pi_f_twice = pi_twice<I_f>();

  I_d pi_d_half = pi_half<I_d>();
  I_d pi_d_twice = pi_twice<I_d>();

  I_ld pi_ld_half = pi_half<I_ld>();
  I_ld pi_ld_twice = pi_twice<I_ld>();

  BOOST_CHECK(equal(2.0f * pi_f_half, pi_f));
  BOOST_CHECK(equal(2.0  * pi_d_half, pi_d));
  BOOST_CHECK(equal(2.0l * pi_ld_half, pi_ld));

  BOOST_CHECK(equal(2.0f * pi_f, pi_f_twice));
  BOOST_CHECK(equal(2.0  * pi_d, pi_d_twice));
  BOOST_CHECK(equal(2.0l * pi_ld, pi_ld_twice));

  return 0;
}
