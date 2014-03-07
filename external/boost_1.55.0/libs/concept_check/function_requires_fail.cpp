// (C) Copyright Jeremy Siek 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifdef NDEBUG
#  undef NDEBUG
#endif

#include <boost/concept_check.hpp>

/*

  This file verifies that function_requires() of the Boost Concept
  Checking Library catches errors when it is suppose to.

*/

struct foo { };

int
main()
{
    boost::function_requires< boost::EqualityComparable<foo> >();
    return 0;
}
