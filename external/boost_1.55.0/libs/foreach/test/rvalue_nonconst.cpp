//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
  Revision history:
  25 August 2005 : Initial version.
*/

#include <vector>
#include <boost/test/minimal.hpp>
#include <boost/foreach.hpp>

#ifdef BOOST_FOREACH_NO_RVALUE_DETECTION
# error Expected failure : rvalues disallowed
#else

std::vector<int> get_vector()
{
    return std::vector<int>(4, 4);
}

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
int test_main( int, char*[] )
{
    int counter = 0;

    BOOST_FOREACH(int i, get_vector())
    {
        counter += i;
    }

    BOOST_CHECK(16 == counter);

    return 0;
}

#endif
