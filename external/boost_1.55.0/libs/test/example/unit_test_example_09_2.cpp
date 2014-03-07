//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
// only one file should define BOOST_TEST_MAIN/BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>

// STL
#include <iostream>

//____________________________________________________________________________//

struct MyConfig2 { 
    MyConfig2() { std::cout << "global setup part2\n"; } 
    ~MyConfig2() { std::cout << "global teardown part2\n"; } 
};

// structure MyConfig2 is used as a global fixture. You could have any number of global fxtures
BOOST_GLOBAL_FIXTURE( MyConfig2 )

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( my_test2 )
{
    BOOST_CHECK( true );
}

//____________________________________________________________________________//

// EOF
