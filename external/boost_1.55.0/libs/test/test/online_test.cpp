//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : included Test Execution Monitor test
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE( test ) 
{
    int i = 1;
    BOOST_CHECK_EQUAL( i*i, 1 );
}

//____________________________________________________________________________//

// EOF
