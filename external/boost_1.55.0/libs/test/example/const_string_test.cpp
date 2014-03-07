//  (C) Copyright Gennadiy Rozental 2001-2005.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile: const_string_test.cpp,v $
//
//  Version     : $Revision: 49313 $
//
//  Description : simple string class test
// ***************************************************************************

#define BOOST_TEST_MODULE const_string test
#include <boost/test/included/unit_test.hpp>

#include "const_string.hpp"
using common_layer::const_string;

BOOST_AUTO_TEST_CASE( constructors_test )
{
    const_string cs0( "" );
    BOOST_CHECK_EQUAL( cs0.length(), (size_t)0 );
    BOOST_CHECK_EQUAL( cs0.begin(), "" );
    BOOST_CHECK_EQUAL( cs0.end(), "" );
    BOOST_CHECK( cs0.is_empty() );

    const_string cs01( NULL );
    BOOST_CHECK_EQUAL( cs01.length(), (size_t)0 );
    BOOST_CHECK_EQUAL( cs01.begin(), "" );
    BOOST_CHECK_EQUAL( cs01.end(), "" );
    BOOST_CHECK( cs01.is_empty() );

    const_string cs1( "test_string" );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );
    BOOST_CHECK_EQUAL( cs1.length(), std::strlen("test_string") );

    std::string  s( "test_string" );
    const_string cs2( s );
    BOOST_CHECK_EQUAL( std::strcmp( cs2.data(), "test_string" ), 0 );

    const_string cs3( cs1 );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );

    const_string cs4( "test_string", 4 );
    BOOST_CHECK_EQUAL( std::strncmp( cs4.data(), "test", cs4.length() ), 0 );

    const_string cs5( s.data(), s.data() + s.length() );
    BOOST_CHECK_EQUAL( std::strncmp( cs5.data(), "test_string", cs5.length() ), 0 );

    const_string cs_array[] = { "str1", "str2" };

    BOOST_CHECK_EQUAL( cs_array[0], "str1" );
    BOOST_CHECK_EQUAL( cs_array[1], "str2" );
}

BOOST_AUTO_TEST_CASE( data_access_test )
{
    const_string cs1( "test_string" );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );

    BOOST_CHECK_EQUAL( cs1[(size_t)0], 't' );
    BOOST_CHECK_EQUAL( cs1[(size_t)4], '_' );
    BOOST_CHECK_EQUAL( cs1[cs1.length()-1], 'g' );

    BOOST_CHECK_EQUAL( cs1[(size_t)0], cs1.at( 0 ) );
    BOOST_CHECK_EQUAL( cs1[(size_t)2], cs1.at( 5 ) );
    BOOST_CHECK_EQUAL( cs1.at( cs1.length() - 1 ), 'g' );

    BOOST_CHECK_THROW( cs1.at( cs1.length() ), std::out_of_range );

    BOOST_CHECK_EQUAL( common_layer::first_char()( cs1  ), 't' );
    BOOST_CHECK_EQUAL( common_layer::last_char()( cs1  ) , 'g' );
}


BOOST_AUTO_TEST_CASE( length_test )
{
    const_string cs1;

    BOOST_CHECK_EQUAL( cs1.length(), (size_t)0 );
    BOOST_CHECK( cs1.is_empty() );

    cs1 = "";
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)0 );
    BOOST_CHECK( cs1.is_empty() );

    cs1 = "test_string";
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)11 );

    cs1.erase();
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)0 );
    BOOST_CHECK_EQUAL( cs1.data(), "" );

    cs1 = const_string( "test_string", 4 );
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)4 );

    cs1.resize( 5 );
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)4 );

    cs1.resize( 3 );
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)3 );

    cs1.rshorten();
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)2 );
    BOOST_CHECK_EQUAL( cs1[(size_t)0], 't' );

    cs1.lshorten();
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)1 );
    BOOST_CHECK_EQUAL( cs1[(size_t)0], 'e' );

    cs1.lshorten();
    BOOST_CHECK( cs1.is_empty() );
    BOOST_CHECK_EQUAL( cs1.data(), "" );

    cs1 = "test_string";
    cs1.lshorten( 11 );
    BOOST_CHECK( cs1.is_empty() );
    BOOST_CHECK_EQUAL( cs1.data(), "" );
}

BOOST_AUTO_TEST_CASE( asignment_test )
{
    const_string cs1;
    std::string  s( "test_string" );

    cs1 = "test";
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test" ), 0 );

    cs1 = s;
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );

    cs1.assign( "test" );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test" ), 0 );

    const_string cs2( "test_string" );

    cs1.swap( cs2 );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );
    BOOST_CHECK_EQUAL( std::strcmp( cs2.data(), "test" ), 0 );
}

BOOST_AUTO_TEST_CASE( comparison_test )
{
    const_string cs1( "test_string" );
    const_string cs2( "test_string" );
    std::string  s( "test_string" );

    BOOST_CHECK_EQUAL( cs1, "test_string" );
    BOOST_CHECK_EQUAL( "test_string", cs1 );
    BOOST_CHECK_EQUAL( cs1, cs2 );
    BOOST_CHECK_EQUAL( cs1, s );
    BOOST_CHECK_EQUAL( s  , cs1 );

    cs1.resize( 4 );

    BOOST_CHECK( cs1 != "test_string" );
    BOOST_CHECK( "test_string" != cs1 );
    BOOST_CHECK( cs1 != cs2 );
    BOOST_CHECK( cs1 != s );
    BOOST_CHECK( s   != cs1 );

    BOOST_CHECK_EQUAL( cs1, "test" );
}

BOOST_AUTO_TEST_CASE( iterators_test )
{
    const_string cs1( "test_string" );
    std::string  s;

    std::copy( cs1.begin(), cs1.end(), std::back_inserter( s ) );
    BOOST_CHECK_EQUAL( cs1, s );

    s.erase();

    std::copy( cs1.rbegin(), cs1.rend(), std::back_inserter( s ) );
    BOOST_CHECK_EQUAL( const_string( s ), "gnirts_tset" );
}

// EOF
