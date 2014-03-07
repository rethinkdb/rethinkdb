/* Boost test/overflow.cpp
 * test if extended precision exponent does not disturb interval computation
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

template<class I>
void test_one(typename I::base_type x, typename I::base_type f) {
  I y = x;
  typename I::base_type g = 1 / f;
  const int nb = 10000;
  for(int i = 0; i < nb; i++) y *= f;
  for(int i = 0; i < nb; i++) y *= g;
  BOOST_CHECK(in(x, y));
# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(nb);
# endif
}

template<class I>
void test() {
  test_one<I>(1., 25.);
  test_one<I>(1., 0.04);
  test_one<I>(-1., 25.);
  test_one<I>(-1., 0.04);
}

int test_main(int, char *[]) {
  test<boost::numeric::interval<float> >();
  test<boost::numeric::interval<double> >();
  //test<boost::numeric::interval<long double> >();
# ifdef __BORLANDC__
  ::detail::ignore_warnings();
# endif
  return 0;
}
