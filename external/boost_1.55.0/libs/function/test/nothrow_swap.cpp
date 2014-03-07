// Boost.Function library

//  Copyright Douglas Gregor 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/function.hpp>

struct tried_to_copy { };

struct MaybeThrowOnCopy {
  MaybeThrowOnCopy(int value = 0) : value(value) { }

  MaybeThrowOnCopy(const MaybeThrowOnCopy& other) : value(other.value) {
    if (throwOnCopy)
      throw tried_to_copy();
  }

  MaybeThrowOnCopy& operator=(const MaybeThrowOnCopy& other) {
    if (throwOnCopy)
      throw tried_to_copy();
    value = other.value;
    return *this;
  }

  int operator()() { return value; }

  int value;

  // Make sure that this function object doesn't trigger the
  // small-object optimization in Function.
  float padding[100];

  static bool throwOnCopy;
};

bool MaybeThrowOnCopy::throwOnCopy = false;

int test_main(int, char* [])
{
  boost::function0<int> f;
  boost::function0<int> g;

  MaybeThrowOnCopy::throwOnCopy = false;
  f = MaybeThrowOnCopy(1);
  g = MaybeThrowOnCopy(2);
  BOOST_CHECK(f() == 1);
  BOOST_CHECK(g() == 2);

  MaybeThrowOnCopy::throwOnCopy = true;
  f.swap(g);
  BOOST_CHECK(f() == 2);
  BOOST_CHECK(g() == 1);
  
  return 0;
}
