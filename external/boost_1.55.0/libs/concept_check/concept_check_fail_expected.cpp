// (C) Copyright Jeremy Siek, David Abrahams 2000-2006.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifdef NDEBUG
#  undef NDEBUG
#endif

#include <boost/concept_check.hpp>

/*

  This file verifies that BOOST_CONCEPT_ASSERT catches errors in
  function context.

*/

struct foo { };

int
main()
{
    BOOST_CONCEPT_ASSERT((boost::EqualityComparable<foo>));
    return 0;
}
