// Copyright Douglas Gregor 2002-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/logic/tribool.hpp>
#include <boost/test/minimal.hpp>
#include <iostream>

BOOST_TRIBOOL_THIRD_STATE(maybe)

int test_main(int,char*[])
{
  using namespace boost::logic;

  tribool x; // false
  tribool y(true); // true
  tribool z(maybe); // maybe

  BOOST_CHECK(!x);
  BOOST_CHECK(x == false);
  BOOST_CHECK(false == x);
  BOOST_CHECK(x != true);
  BOOST_CHECK(true != x);
  BOOST_CHECK(maybe(x == maybe));
  BOOST_CHECK(maybe(maybe == x));
  BOOST_CHECK(maybe(x != maybe));
  BOOST_CHECK(maybe(maybe != x));
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
  BOOST_CHECK(maybe(y == maybe));
  BOOST_CHECK(maybe(maybe == y));
  BOOST_CHECK(maybe(y != maybe));
  BOOST_CHECK(maybe(maybe != y));
  BOOST_CHECK(y == y);
  BOOST_CHECK(!(y != y));

  BOOST_CHECK(maybe(z || !z));
  BOOST_CHECK(maybe(z == true));
  BOOST_CHECK(maybe(true == z));
  BOOST_CHECK(maybe(z == false));
  BOOST_CHECK(maybe(false == z));
  BOOST_CHECK(maybe(z == maybe));
  BOOST_CHECK(maybe(maybe == z));
  BOOST_CHECK(maybe(z != maybe));
  BOOST_CHECK(maybe(maybe != z));
  BOOST_CHECK(maybe(z == z));
  BOOST_CHECK(maybe(z != z));

  BOOST_CHECK(!(x == y));
  BOOST_CHECK(x != y);
  BOOST_CHECK(maybe(x == z));
  BOOST_CHECK(maybe(x != z));
  BOOST_CHECK(maybe(y == z));
  BOOST_CHECK(maybe(y != z));

  BOOST_CHECK(!(x && y));
  BOOST_CHECK(x || y);
  BOOST_CHECK(!(x && z));
  BOOST_CHECK(maybe(y && z));
  BOOST_CHECK(maybe(z && z));
  BOOST_CHECK(maybe(z || z));
  BOOST_CHECK(maybe(x || z));
  BOOST_CHECK(y || z);

  BOOST_CHECK(maybe(y && maybe));
  BOOST_CHECK(maybe(maybe && y));
  BOOST_CHECK(!(x && maybe));
  BOOST_CHECK(!(maybe && x));

  BOOST_CHECK(maybe || y);
  BOOST_CHECK(y || maybe);
  BOOST_CHECK(maybe(x || maybe));
  BOOST_CHECK(maybe(maybe || x));

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
