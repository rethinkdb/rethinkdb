// (C) Copyright Jeremy Siek, David Abrahams 2000-2006.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Change Log:
// 20 Jan 2001 - Added warning suppression (David Abrahams)

#include "old_concepts.hpp"

// This file verifies that concepts written the old way still catch
// errors in class context.  This is not expected to work on compilers
// without SFINAE support.

struct foo { };

class class_requires_test
{
  BOOST_CLASS_REQUIRE(foo, old, EqualityComparableConcept);
};

int
main()
{
  class_requires_test x;
  (void)x; // suppress unused variable warning
  return 0;
}
