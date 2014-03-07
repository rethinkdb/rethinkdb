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

// counter
int counter = 0;

std::vector<int> my_vector(4,4);

std::vector<int> const &get_vector()
{
    ++counter;
    return my_vector;
}


///////////////////////////////////////////////////////////////////////////////
// test_main
//   
int test_main( int, char*[] )
{
    BOOST_FOREACH(int i, get_vector())
    {
        ((void)i); // no-op
    }

    BOOST_CHECK(1 == counter);

    return 0;
}
