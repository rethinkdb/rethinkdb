// (C) Copyright Jeremy Siek, David Abrahams 2000-2006.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Change Log:
// 20 Jan 2001 - Added warning suppression (David Abrahams)

#include <boost/concept_check.hpp>

/*

  This file verifies that class_requires of the Boost Concept Checking
  Library catches errors when it is supposed to.

*/

struct foo { };

template <class T>
class class_requires_test
{
    BOOST_CONCEPT_ASSERT((boost::EqualityComparable<foo>));
};

int
main()
{
    class_requires_test<int> x;
    (void)x; // suppress unused variable warning
    return 0;
}
