//  Boost string_algo library trim_test.cpp file  ---------------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/trim_all.hpp>

// Include unit test framework
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <boost/test/test_tools.hpp>

using namespace std;
using namespace boost;

void trim_test()
{
    string str1("     1x x x x1     ");
    string str2("     2x x x x2     ");
    string str3("    ");

    // *** value passing tests *** //

    // general string test
    BOOST_CHECK( trim_left_copy( str1 )=="1x x x x1     " ) ;
    BOOST_CHECK( trim_right_copy( str1 )=="     1x x x x1" ) ;
    BOOST_CHECK( trim_copy( str1 )=="1x x x x1" ) ;

    // spaces-only string test
    BOOST_CHECK( trim_left_copy( str3 )=="" );
    BOOST_CHECK( trim_right_copy( str3 )=="" );
    BOOST_CHECK( trim_copy( str3 )=="" );

    // empty string check 
    BOOST_CHECK( trim_left_copy( string("") )=="" );
    BOOST_CHECK( trim_right_copy( string("") )=="" );
    BOOST_CHECK( trim_copy( string("") )=="" );

    // iterator tests
    string str;
    trim_left_copy_if( std::back_inserter(str), str1, is_space() );
    BOOST_CHECK( str=="1x x x x1     " );

    str.clear();
    trim_right_copy_if( std::back_inserter(str), str1, is_space() );
    BOOST_CHECK( str=="     1x x x x1" );

    str.clear();
    trim_copy_if( std::back_inserter(str), str1, is_space() );
    BOOST_CHECK( str=="1x x x x1" );

    str.clear();
    trim_left_copy_if( 
        std::back_inserter(str), 
        "     1x x x x1     ", 
        is_space() );
    BOOST_CHECK( str=="1x x x x1     " );

    str.clear();
    trim_right_copy_if( 
        std::back_inserter(str), 
        "     1x x x x1     ", 
        is_space() );
    BOOST_CHECK( str=="     1x x x x1" );

    str.clear();
    trim_copy_if( 
        std::back_inserter(str), 
        "     1x x x x1     ", 
        is_space() );
    BOOST_CHECK( str=="1x x x x1" );
    // *** inplace tests *** //

    // general string test
    trim_left( str1 );
    BOOST_CHECK( str1=="1x x x x1     " );
    trim_right( str1 );
    BOOST_CHECK( str1=="1x x x x1" );
    trim( str2 );
    BOOST_CHECK( str2=="2x x x x2" );
    
    // spaces-only string test
    str3 = "    "; trim_left( str3 );
    BOOST_CHECK( str3=="" );
    str3 = "    "; trim_right( str3 );
    BOOST_CHECK( str3=="" );
    str3 = "    "; trim( str3 );
    BOOST_CHECK( str3=="" );

    // empty string check 
    str3 = ""; trim_left( str3 );
    BOOST_CHECK( str3=="" );
    str3 = ""; trim_right( str3 );
    BOOST_CHECK( str3=="" );
    str3 = ""; trim( str3 );
    BOOST_CHECK( str3=="" );

    // *** non-standard predicate tests *** //
    BOOST_CHECK( 
        trim_copy_if( 
            string("123abc456"), 
            is_classified(std::ctype_base::digit) )=="abc" );
    BOOST_CHECK( trim_copy_if( string("<>abc<>"), is_any_of( "<<>>" ) )=="abc" );
}

void trim_all_test()
{
    string str1("     1x   x   x   x1     ");
    string str2("+---...2x+--x--+x-+-x2...---+");
    string str3("    ");

    // *** value passing tests *** //

    // general string test
    BOOST_CHECK( trim_all_copy( str1 )=="1x x x x1" ) ;
    BOOST_CHECK( trim_all_copy_if( str2, is_punct() )=="2x+x-x-x2" ) ;

    // spaces-only string test
    BOOST_CHECK( trim_all_copy( str3 )=="" );

    // empty string check 
    BOOST_CHECK( trim_all_copy( string("") )=="" );

    // general string test
    trim_all( str1 );
    BOOST_CHECK( str1=="1x x x x1" ) ;
    trim_all_if( str2, is_punct() );
    BOOST_CHECK( str2=="2x+x-x-x2" ) ;
    
    // spaces-only string test
    str3 = "    "; trim_all( str3 );
    BOOST_CHECK( str3=="" );

    // empty string check 
    str3 = ""; trim_all( str3 );
    BOOST_CHECK( str3=="" );
    BOOST_CHECK( str3=="" );

    // *** non-standard predicate tests *** //
    BOOST_CHECK( 
        trim_all_copy_if( 
            string("123abc127deb456"), 
            is_classified(std::ctype_base::digit) )=="abc1deb" );
    BOOST_CHECK( trim_all_copy_if( string("<>abc<>def<>"), is_any_of( "<<>>" ) )=="abc<def" );
}

void trim_fill_test()
{
    string str1("     1x   x   x   x1     ");
    string str2("+---...2x+--x--+x-+-x2...---+");
    string str3("    ");

    // *** value passing tests *** //

    // general string test
    BOOST_CHECK( trim_fill_copy( str1, "-" )=="1x-x-x-x1" ) ;
    BOOST_CHECK( trim_fill_copy_if( str2, " ", is_punct() )=="2x x x x2" ) ;

    // spaces-only string test
    BOOST_CHECK( trim_fill_copy( str3, " " )=="" );

    // empty string check 
    BOOST_CHECK( trim_fill_copy( string(""), " " )=="" );

    // general string test
    trim_fill( str1, "-" );
    BOOST_CHECK( str1=="1x-x-x-x1" ) ;
    trim_fill_if( str2, "", is_punct() );
    BOOST_CHECK( str2=="2xxxx2" ) ;

    // spaces-only string test
    str3 = "    "; trim_fill( str3, "" );
    BOOST_CHECK( str3=="" );

    // empty string check 
    str3 = ""; trim_fill( str3, "" );
    BOOST_CHECK( str3=="" );
    BOOST_CHECK( str3=="" );

    // *** non-standard predicate tests *** //
    BOOST_CHECK( 
        trim_fill_copy_if( 
        string("123abc127deb456"), 
        "+",
        is_classified(std::ctype_base::digit) )=="abc+deb" );
    BOOST_CHECK( trim_fill_copy_if( string("<>abc<>def<>"), "-", is_any_of( "<<>>" ) )=="abc-def" );
}

BOOST_AUTO_TEST_CASE( test_main )
{
    trim_test();
    trim_all_test();
    trim_fill_test();
}
