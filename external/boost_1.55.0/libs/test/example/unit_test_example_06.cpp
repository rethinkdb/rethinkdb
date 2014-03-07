//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#define BOOST_TEST_MODULE Unit test example 06
#include <boost/test/unit_test.hpp>

//____________________________________________________________________________//

struct F {
    F() : i( 0 ) { BOOST_TEST_MESSAGE( "setup fixture" ); }
    ~F()         { BOOST_TEST_MESSAGE( "teardown fixture" ); }
   
    int i;
};

//____________________________________________________________________________//

// struct F is going to be used as a fixture for all test cases in this test suite
BOOST_FIXTURE_TEST_SUITE( s, F )

BOOST_AUTO_TEST_CASE( my_test1 )
{
    BOOST_CHECK( i == 1 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( my_test2 )
{
    BOOST_CHECK_EQUAL( i, 2 );

    BOOST_CHECK_EQUAL( i, 0 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

// EOF
