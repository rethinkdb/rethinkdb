/* Boost test/integer.cpp
 * test int extension
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <boost/numeric/interval/ext/integer.hpp>
#include "bugs.hpp"

typedef boost::numeric::interval<float> I;

int main() {
  I x, y;
  x = 4 - (2 * y + 1) / 3;
# ifdef __BORLANDC__
  ::detail::ignore_warnings();
# endif
  return 0;
}
