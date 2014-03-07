// (C) Copyright Jeremy Siek, David Abrahams 2000-2006.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/concept_check.hpp>
#include "old_concepts.hpp"

// This test verifies that use of the old-style concept checking
// classes still compiles (but not that it detects constraint
// violations).  We check them with the old-style macros just for
// completeness, since those macros stranslate into
// BOOST_CONCEPT_ASSERTs.

struct foo { bool operator()(int) { return true; } };
struct bar { bool operator()(int, char) { return true; } };


class class_requires_test
{
  BOOST_CLASS_REQUIRE(int, old, EqualityComparableConcept);
  typedef int* int_ptr; typedef const int* const_int_ptr;
  BOOST_CLASS_REQUIRE2(int_ptr, const_int_ptr, old, EqualOpConcept);
  BOOST_CLASS_REQUIRE3(foo, bool, int, old, UnaryFunctionConcept);
  BOOST_CLASS_REQUIRE4(bar, bool, int, char, old, BinaryFunctionConcept);
};

int
main()
{
    class_requires_test x;
    boost::ignore_unused_variable_warning(x);
    return 0;
}
