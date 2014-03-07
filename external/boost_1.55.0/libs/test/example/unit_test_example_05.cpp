//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#define BOOST_TEST_MODULE "Unit test example 05"
#include <boost/test/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE( my_suite )

struct F {
    F() : i( 0 ) { BOOST_TEST_MESSAGE( "setup fixture" ); }
    ~F()         { BOOST_TEST_MESSAGE( "teardown fixture" ); }
   
    int i;
};

//____________________________________________________________________________//

// this test case will use struct F as fixture
BOOST_FIXTURE_TEST_CASE( my_test1, F )
{
    // you have direct access to non-private members of fixture structure
    BOOST_CHECK( i == 1 );
}

//____________________________________________________________________________//

// you could have any number of test cases with the same fixture
BOOST_FIXTURE_TEST_CASE( my_test2, F )
{
    BOOST_CHECK_EQUAL( i, 2 );

    BOOST_CHECK_EQUAL( i, 0 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

// EOF
