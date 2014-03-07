//  (C) Copyright Gennadiy Rozental 2005-2007.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#ifndef BOOST_TEST_DYN_LINK 
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test1 )
{
    int i = 0;

    BOOST_CHECK_EQUAL( i, 2 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2 )
{
    BOOST_CHECKPOINT("About to force division by zero!");
    int i = 1, j = 0;

    i = i / j;
}

//____________________________________________________________________________//

extern "C" {

#ifdef BOOST_WINDOWS
__declspec(dllexport)
#endif
 bool
init_unit_test()
{
    framework::master_test_suite().p_name.value = "Test runner test";

    return true;
}

}

//____________________________________________________________________________//

// EOF
