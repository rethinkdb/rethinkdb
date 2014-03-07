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
//  Description : ifstream_line_iterator unit test
// *****************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/test/utils/iterator/ifstream_line_iterator.hpp>

namespace ut = boost::unit_test;

static ut::ifstream_line_iterator eoi;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_default_delimeter )
{
    ut::ifstream_line_iterator it( ut::framework::master_test_suite().argc <= 1
                                        ? "./test_files/ifstream_line_iterator.tst1"
                                        : ut::framework::master_test_suite().argv[1] );

    BOOST_CHECK( it != eoi ); 

    BOOST_CHECK_EQUAL( *it, "acv ffg" ); 
    ++it;

    BOOST_CHECK_EQUAL( *it, "" ); 
    ++it;

    BOOST_CHECK_EQUAL( *it, " " ); 
    ++it;

    BOOST_CHECK_EQUAL( *it, "1" ); 
    ++it;

    BOOST_CHECK( it == eoi ); 
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_custom_delimeter )
{
    ut::ifstream_line_iterator it( ut::framework::master_test_suite().argc <= 2 
                                        ? "./test_files/ifstream_line_iterator.tst2"
                                        : ut::framework::master_test_suite().argv[2], '}' );

    BOOST_CHECK( it != eoi ); 

    BOOST_CHECK_EQUAL( *it, "{ abc d " ); 
    ++it;

    BOOST_CHECK_EQUAL( *it, "\n{ d \n dsfg\n" ); 
    ++it;

    BOOST_CHECK_EQUAL( *it, "\n" ); 
    ++it;

    BOOST_CHECK( it == eoi ); 
}


//____________________________________________________________________________//

// EOF
