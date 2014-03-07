//  boost utility cast test program  -----------------------------------------//

//  (C) Copyright Beman Dawes, Dave Abrahams 1999. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

//  Revision History
//   28 Set 04  taken from the old cast library (Fernando Cacciola)

#include <iostream>
#include <climits>
#include <cfloat>   // for DBL_MAX (Peter Schmid)

#include <boost/numeric/conversion/cast.hpp>

#include "boost/test/minimal.hpp"

#  if SCHAR_MAX == LONG_MAX
#      error "This test program doesn't work if SCHAR_MAX == LONG_MAX"
#  endif

using namespace boost;
using std::cout;

int test_main( int argc, char * argv[] )
{

#   ifdef NDEBUG
        cout << "NDEBUG is defined\n";
#   else
        cout << "NDEBUG is not defined\n";
#   endif

    cout << "\nBeginning tests...\n";

//  test implicit_cast and numeric_cast  -------------------------------------//

    //  tests which should succeed
    long small_value = 1;
    long small_negative_value = -1;
    long large_value = LONG_MAX;
    long large_negative_value = LONG_MIN;
    signed char c = 0;

    c = large_value;  // see if compiler generates warning

    c = numeric_cast<signed char>( small_value );
    BOOST_CHECK( c == 1 );
    c = 0;
    c = numeric_cast<signed char>( small_value );
    BOOST_CHECK( c == 1 );
    c = 0;
    c = numeric_cast<signed char>( small_negative_value );
    BOOST_CHECK( c == -1 );

    // These tests courtesy of Joe R NWP Swatosh<joe.r.swatosh@usace.army.mil>
    BOOST_CHECK( 0.0f == numeric_cast<float>( 0.0 ) );
    BOOST_CHECK( 0.0 == numeric_cast<double>( 0.0 ) );

    //  tests which should result in errors being detected

    bool caught_exception = false;
    try { c = numeric_cast<signed char>( large_value ); }
    catch ( numeric::bad_numeric_cast )
        { cout<<"caught bad_numeric_cast #1\n"; caught_exception = true; }
    BOOST_CHECK ( caught_exception );

    caught_exception = false;
    try { c = numeric_cast<signed char>( large_negative_value ); }
    catch ( numeric::bad_numeric_cast )
        { cout<<"caught bad_numeric_cast #2\n"; caught_exception = true; }
    BOOST_CHECK ( caught_exception );

    unsigned long ul;
    caught_exception = false;
    try { ul = numeric_cast<unsigned long>( large_negative_value ); }
    catch ( numeric::bad_numeric_cast )
        { cout<<"caught bad_numeric_cast #3\n"; caught_exception = true; }
    BOOST_CHECK ( caught_exception );

    caught_exception = false;
    try { ul = numeric_cast<unsigned long>( small_negative_value ); }
    catch ( numeric::bad_numeric_cast )
        { cout<<"caught bad_numeric_cast #4\n"; caught_exception = true; }
    BOOST_CHECK ( caught_exception );

    caught_exception = false;
    try { numeric_cast<int>( DBL_MAX ); }
    catch ( numeric::bad_numeric_cast )
        { cout<<"caught bad_numeric_cast #5\n"; caught_exception = true; }
    BOOST_CHECK ( caught_exception );

    return 0 ;

}   // main
