//  (C) Copyright Gennadiy Rozental 2003-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : unit test for class properties facility
// ***************************************************************************

// Boost.Test
#include <boost/test/unit_test.hpp>
#include <boost/test/utils/class_properties.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/test/utils/algorithm.hpp>
namespace utf = boost::unit_test;
using utf::const_string;

// STL
#include <cctype>
#include <functional>

# ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::toupper; }
# endif

//____________________________________________________________________________//

bool predicate( char c1, char c2 ) { return (std::toupper)( c1 ) == (std::toupper)( c2 ); }

//____________________________________________________________________________//

void test_mismatch()
{
    const_string cs1( "test_string" );
    const_string cs2( "test_stream" );

    BOOST_CHECK_EQUAL( utf::mismatch( cs1.begin(), cs1.end(), cs2.begin(), cs2.end() ).first - cs1.begin(), 8 );

    cs2 = "trest";
    BOOST_CHECK_EQUAL( utf::mismatch( cs1.begin(), cs1.end(), cs2.begin(), cs2.end() ).first - cs1.begin(), 1 );

    cs2 = "test_string_klmn";
    BOOST_CHECK_EQUAL( utf::mismatch( cs1.begin(), cs1.end(), cs2.begin(), cs2.end() ).first - cs1.begin(), 11 );

    cs2 = "TeSt_liNk";
    BOOST_CHECK_EQUAL( 
        utf::mismatch( cs1.begin(), cs1.end(), cs2.begin(), cs2.end(), std::ptr_fun( predicate ) ).first - cs1.begin(),
        5 );
}

//____________________________________________________________________________//

void test_find_first_not_of()
{
    const_string cs( "test_string" );
    const_string another( "tes" );

    BOOST_CHECK_EQUAL( utf::find_first_not_of( cs.begin(), cs.end(), another.begin(), another.end() ) - cs.begin(), 4 );

    another = "T_sE";
    BOOST_CHECK_EQUAL( 
        utf::find_first_not_of( cs.begin(), cs.end(), another.begin(), another.end(), std::ptr_fun( predicate ) ) - cs.begin(), 
        7 );

    another = "tes_ring";
    BOOST_CHECK( utf::find_last_not_of( cs.begin(), cs.end(), another.begin(), another.end() ) == cs.end() );
}

//____________________________________________________________________________//

void test_find_last_of()
{
    const_string cs( "test_string" );
    const_string another( "tes" );

    BOOST_CHECK_EQUAL( utf::find_last_of( cs.begin(), cs.end(), another.begin(), another.end() ) - cs.begin(), 6 );

    another = "_Se";
    BOOST_CHECK_EQUAL( 
        utf::find_last_of( cs.begin(), cs.end(), another.begin(), another.end(), std::ptr_fun( predicate ) ) - cs.begin(),
        5 );

    another = "qw";
    BOOST_CHECK( utf::find_last_of( cs.begin(), cs.end(), another.begin(), another.end() ) == cs.end() );

    cs = "qerty";
    BOOST_CHECK_EQUAL( utf::find_last_of( cs.begin(), cs.end(), another.begin(), another.end() ) - cs.begin(), 0 );
}

//____________________________________________________________________________//

void test_find_last_not_of()
{
    const_string cs( "test_string" );
    const_string another( "string" );

    BOOST_CHECK_EQUAL( utf::find_last_not_of( cs.begin(), cs.end(), another.begin(), another.end() ) - cs.begin(), 4 );

    another = "_SeG";
    BOOST_CHECK_EQUAL( 
        utf::find_last_not_of( cs.begin(), cs.end(), another.begin(), another.end(), std::ptr_fun( predicate ) ) - cs.begin(),
        9 );

    another = "e_string";
    BOOST_CHECK( utf::find_last_not_of( cs.begin(), cs.end(), another.begin(), another.end() ) == cs.end() );
}

//____________________________________________________________________________//

utf::test_suite*
init_unit_test_suite( int /* argc */, char* /* argv */ [] )
{
    utf::test_suite* test= BOOST_TEST_SUITE("Algorithms test");

    test->add( BOOST_TEST_CASE( test_mismatch ) );
    test->add( BOOST_TEST_CASE( test_find_first_not_of ) );
    test->add( BOOST_TEST_CASE( test_find_last_of ) );
    test->add( BOOST_TEST_CASE( test_find_last_not_of ) );

    return test;
}

//____________________________________________________________________________//

// EOF
