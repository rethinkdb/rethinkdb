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

// you could easily implement test cases as a free functions
// this test case will need to be explecetely registered in test tree
void free_test_function()
{
    // reports 'error in "free_test_function": test 2 == 1 failed'
    BOOST_CHECK(2 == 1); // non-critical test => continue after failure

    // reports 'unknown location(0): fatal error in "free_test_function": memory access violation
    //          d:\source code\boost\libs\test\example\unit_test_example_02.cpp(25): last checkpoint'
    int* p = (int*)0x01;
    BOOST_CHECK( *p == 0 );
}

//____________________________________________________________________________//


test_suite*
init_unit_test_suite( int, char* [] ) {
    framework::master_test_suite().p_name.value = "Unit test example 02";

    // register the test case in test tree and specify number of expected failures so
    // this example will pass at runtime. We expect 2 errors: one from failed check and 
    // one from memory acces violation
    framework::master_test_suite().add( BOOST_TEST_CASE( &free_test_function ), 2 );

    return 0;
}

//____________________________________________________________________________//

// EOF
