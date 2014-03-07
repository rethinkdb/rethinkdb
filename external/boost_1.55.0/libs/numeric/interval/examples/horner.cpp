/* Boost example/horner.cpp
 * example of unprotecting rounding for a whole function computation
 *
 * Copyright 2002-2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <iostream>

// I is an interval class, the polynom is a simple array
template<class I>
I horner(const I& x, const I p[], int n) {

  // initialize and restore the rounding mode
  typename I::traits_type::rounding rnd;

  // define the unprotected version of the interval type
  typedef typename boost::numeric::interval_lib::unprotect<I>::type R;

  const R& a = x;
  R y = p[n - 1];
  for(int i = n - 2; i >= 0; i--) {
    y = y * a + (const R&)(p[i]);
  }
  return y;

  // restore the rounding mode with the destruction of rnd
}

template<class T, class Policies>
std::ostream &operator<<(std::ostream &os,
                         const boost::numeric::interval<T, Policies> &x) {
  os << "[" << x.lower() << ", " << x.upper() << "]";
  return os;
}

int main() {
  typedef boost::numeric::interval<double> I;
  I p[3] = { -1.0, 0, 1.0 };
  I x = 1.0;
  std::cout << horner(x, p, 3) << std::endl;
  return 0;
}
