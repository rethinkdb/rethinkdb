// (C) Copyright Jeremy Siek 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifdef NDEBUG
#  undef NDEBUG
#endif

#include "old_concepts.hpp"

// This file verifies that concepts written the old way still catch
// errors in function context.  This is not expected to work on
// compilers without SFINAE support.

struct foo { };

int
main()
{
    boost::function_requires< old::EqualityComparableConcept<foo> >();
    return 0;
}
