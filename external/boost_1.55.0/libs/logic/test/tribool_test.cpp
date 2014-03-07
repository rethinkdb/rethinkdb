// Copyright Douglas Gregor 2002-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/logic/tribool.hpp>
#include <boost/test/minimal.hpp>
#include <iostream>

int test_main(int, char*[])
{
  using namespace boost::logic;

  tribool x; // false
  tribool y(true); // true
  tribool z(indeterminate); // indeterminate

  BOOST_CHECK(!x);
  BOOST_CHECK(x == false);
  BOOST_CHECK(false == x);
  BOOST_CHECK(x != true);
  BOOST_CHECK(true != x);
  BOOST_CHECK(indeterminate(x == indeterminate));
  BOOST_CHECK(indeterminate(indeterminate == x));
  BOOST_CHECK(indeterminate(x != indeterminate));
  BOOST_CHECK(indeterminate(indeterminate != x));
  BOOST_CHECK(x == x);
  BOOST_CHECK(!(x != x));
  BOOST_CHECK(!(x && true));
  BOOST_CHECK(!(true && x));
  BOOST_CHECK(x || true);
  BOOST_CHECK(true || x);

  BOOST_CHECK(y);
  BOOST_CHECK(y == true);
  BOOST_CHECK(true == y);
  BOOST_CHECK(y != false);
  BOOST_CHECK(false != y);
  BOOST_CHECK(indeterminate(y == indeterminate));
  BOOST_CHECK(indeterminate(indeterminate == y));
  BOOST_CHECK(indeterminate(y != indeterminate));
  BOOST_CHECK(indeterminate(indeterminate != y));
  BOOST_CHECK(y == y);
  BOOST_CHECK(!(y != y));

  BOOST_CHECK(indeterminate(z || !z));
  BOOST_CHECK(indeterminate(z == true));
  BOOST_CHECK(indeterminate(true == z));
  BOOST_CHECK(indeterminate(z == false));
  BOOST_CHECK(indeterminate(false == z));
  BOOST_CHECK(indeterminate(z == indeterminate));
  BOOST_CHECK(indeterminate(indeterminate == z));
  BOOST_CHECK(indeterminate(z != indeterminate));
  BOOST_CHECK(indeterminate(indeterminate != z));
  BOOST_CHECK(indeterminate(z == z));
  BOOST_CHECK(indeterminate(z != z));

  BOOST_CHECK(!(x == y));
  BOOST_CHECK(x != y);
  BOOST_CHECK(indeterminate(x == z));
  BOOST_CHECK(indeterminate(x != z));
  BOOST_CHECK(indeterminate(y == z));
  BOOST_CHECK(indeterminate(y != z));

  BOOST_CHECK(!(x && y));
  BOOST_CHECK(x || y);
  BOOST_CHECK(!(x && z));
  BOOST_CHECK(indeterminate(y && z));
  BOOST_CHECK(indeterminate(z && z));
  BOOST_CHECK(indeterminate(z || z));
  BOOST_CHECK(indeterminate(x || z));
  BOOST_CHECK(y || z);

  BOOST_CHECK(indeterminate(y && indeterminate));
  BOOST_CHECK(indeterminate(indeterminate && y));
  BOOST_CHECK(!(x && indeterminate));
  BOOST_CHECK(!(indeterminate && x));

  BOOST_CHECK(indeterminate || y);
  BOOST_CHECK(y || indeterminate);
  BOOST_CHECK(indeterminate(x || indeterminate));
  BOOST_CHECK(indeterminate(indeterminate || x));

  // Test the if (z) ... else (!z) ... else ... idiom
  if (z) {
    BOOST_CHECK(false);
  }
  else if (!z) {
    BOOST_CHECK(false);
  }
  else {
    BOOST_CHECK(true);
  }

  z = true;
  if (z) {
    BOOST_CHECK(true);
  }
  else if (!z) {
    BOOST_CHECK(false);
  }
  else {
    BOOST_CHECK(false);
  }

  z = false;
  if (z) {
    BOOST_CHECK(false);
  }
  else if (!z) {
    BOOST_CHECK(true);
  }
  else {
    BOOST_CHECK(false);
  }

  std::cout << "no errors detected\n";
  return 0;
}
