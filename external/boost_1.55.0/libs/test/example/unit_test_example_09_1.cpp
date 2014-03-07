//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#define BOOST_TEST_MODULE Unit_test_example_09
#include <boost/test/unit_test.hpp>

// STL
#include <iostream>

//____________________________________________________________________________//

struct MyConfig { 
    MyConfig() { std::cout << "global setup part1\n"; } 
    ~MyConfig() { std::cout << "global teardown part1\n"; } 
};

// structure MyConfig is used as a global fixture - it's invoked pre and post any testing is performed
BOOST_GLOBAL_FIXTURE( MyConfig )

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( my_test1 )
{
    BOOST_CHECK( true );
}

//____________________________________________________________________________//

// EOF
