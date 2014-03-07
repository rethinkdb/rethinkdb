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
//  Description : tests output_test_stream test tool functionality
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
using boost::test_tools::output_test_stream;

// STL
#include <iomanip>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_constructor )
{
    {
        output_test_stream output;
        BOOST_CHECK( !output.match_pattern() );
        BOOST_CHECK( output.is_empty() );
    }
    {
        output_test_stream output( (char const*)0 );
        BOOST_CHECK( !output.match_pattern() );
        BOOST_CHECK( output.is_empty() );
    }
    {
        output_test_stream output( "" );
        BOOST_CHECK( !output.match_pattern() );
        BOOST_CHECK( output.is_empty() );
    }
    {
        output_test_stream output( "%&^$%&$%" );
        BOOST_CHECK( !output.match_pattern() );
        BOOST_CHECK( output.is_empty() );
    }
    {
        output_test_stream output( "pattern.temp" );
        BOOST_CHECK( !output.match_pattern() );
        BOOST_CHECK( output.is_empty() );
    }
    {
        output_test_stream output( "pattern.temp2", false );
        BOOST_CHECK( output.match_pattern() );
        BOOST_CHECK( output.is_empty() );
    }
    {
        output_test_stream output( "pattern.temp2" );
        BOOST_CHECK( output.match_pattern() );
        BOOST_CHECK( output.is_empty() );
    }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_is_empty )
{
    output_test_stream output;
    BOOST_CHECK( output.is_empty() );

    output << 12345;
    BOOST_CHECK( !output.is_empty() );
    BOOST_CHECK( output.is_empty() );

    output << "";
    BOOST_CHECK( output.is_empty() );

    output << '\0';
    BOOST_CHECK( !output.is_empty( false ) );
    BOOST_CHECK( !output.is_empty() );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_check_length )
{
    output_test_stream output;
    BOOST_CHECK( output.check_length( 0 ) );

    output << "";
    BOOST_CHECK( output.check_length( 0 ) );

    output << '\0';
    BOOST_CHECK( output.check_length( 1 ) );

    output << 1220;
    BOOST_CHECK( output.check_length( 4 ) );

    output << "Text message";
    BOOST_CHECK( output.check_length( 12, false ) );
    BOOST_CHECK( output.check_length( 12 ) );

    output.width( 20 );
    output << "Text message";
    BOOST_CHECK( output.check_length( 20 ) );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_is_equal )
{
    output_test_stream output;
    BOOST_CHECK( output.is_equal( "" ) );

    output << 1;
    BOOST_CHECK( output.is_equal( "1" ) );

    output << "";
    BOOST_CHECK( output.is_equal( "" ) );

    output << '\0';
    BOOST_CHECK( output.is_equal( boost::unit_test::const_string( "", 1 ) ) );

    output << std::setw( 10 ) << "qwerty" << '\n';
    BOOST_CHECK( output.is_equal( "    qwerty\n" ) );

    std::string s( "test string" );

    output << s << std::endl;
    BOOST_CHECK( output.is_equal( "test string\n", false ) );
    
    output << s << std::endl;
    BOOST_CHECK( output.is_equal( "test string\ntest string\n" ) );

    char const* literal_string = "asdfghjkl";
    std::string substr1( literal_string, 5 );
    std::string substr2( literal_string+5, 4 );

    output << substr1;
    BOOST_CHECK( output.is_equal( boost::unit_test::const_string( literal_string, 5 ), false ) );

    output << substr2;
    BOOST_CHECK( output.is_equal( boost::unit_test::const_string( literal_string, 9 ) ) );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_match_pattern )
{
    for( int i1 = 0; i1 < 2; i1++ ) {
        output_test_stream output( "pattern.test", i1 == 1 );
        
        output << "text1\n";
        BOOST_CHECK( output.match_pattern() );
        output << "text2\n";
        BOOST_CHECK( output.match_pattern() );
        output << "text3\n";
        BOOST_CHECK( output.match_pattern() );
    }

    {
        output_test_stream output( "pattern.test" );

        output << "text4\n";
        BOOST_CHECK( !output.match_pattern() );
        output << "text2\n";
        BOOST_CHECK( output.match_pattern() );
        output << "text3\n";
        BOOST_CHECK( output.match_pattern() );
    }
    {
        output_test_stream output( "pattern.test" );

        output << "text\n";
        BOOST_CHECK( !output.match_pattern() );
        output << "text2\n";
        BOOST_CHECK( !output.match_pattern() );
        output << "text3\n";
        BOOST_CHECK( !output.match_pattern() );
    }

    for( int i2 = 0; i2 < 2; i2++ ) {
        output_test_stream output( "pattern.test", i2 == 1, false );

        output << "text\rmore text\n";
        BOOST_CHECK( output.match_pattern() );
    }
}

//____________________________________________________________________________//

// EOF
