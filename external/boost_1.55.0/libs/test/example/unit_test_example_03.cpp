//  (C) Copyright Gennadiy Rozental 2002-2008.
//  (C) Copyright Gennadiy Rozental & Ullrich Koethe 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

// this test case is automatically registered
BOOST_AUTO_TEST_CASE( force_division_by_zero )
{
    BOOST_CHECK( false );

    // unit test framework can catch operating system signals
    BOOST_TEST_CHECKPOINT("About to force division by zero!");
    int i = 1, j = 0;

    // reports 'unknown location(0): fatal error in "force_division_by_zero": integer divide by zero'
    i = i / j;
}

//____________________________________________________________________________//

// this test case will have tobe registered manually
void infinite_loop()
{
    // unit test framework can break infinite loops by timeout
#ifdef __unix  // don't have timeout on other platforms
    BOOST_TEST_CHECKPOINT("About to enter an infinite loop!");
    while(1);
#else
    BOOST_TEST_MESSAGE( "Timeout support is not implemented on your platform" );
#endif
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int , char* [] )
{
    framework::master_test_suite().p_name.value = "Unit test example 03";

    // with explicit registration we could specify a test case timeout
    framework::master_test_suite().add( BOOST_TEST_CASE( &infinite_loop ), 0, /* timeout */ 2 );

    return 0; 
}

//____________________________________________________________________________//

// EOF
