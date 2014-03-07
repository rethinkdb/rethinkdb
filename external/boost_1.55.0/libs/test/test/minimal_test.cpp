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
//  Description : minimal testing unit test
// ***************************************************************************

// Boost.Test
#include <boost/test/minimal.hpp>

//____________________________________________________________________________//

struct bool_convertible1 {
    bool_convertible1( bool v ) : v_( v ) {}
    operator bool() { return v_; }

    bool v_;
};

//____________________________________________________________________________//

struct bool_convertible2 {
    bool_convertible2( int v ) : v_( v ) {}
    operator int() { return v_; }

    int v_;
};

//____________________________________________________________________________//

struct bool_convertible3 {
    bool_convertible3( void* v ) : v_( v ) {}

    struct Tester {};
    operator Tester*() { return (Tester*)v_; }

    void* v_;
};

//____________________________________________________________________________//

int 
test_main( int /*argc*/, char* /*argv*/[] ) 
{
    int i = 1;

    BOOST_CHECK( i == 1 );
    BOOST_CHECK( i == 2 );

    BOOST_CHECK( bool_convertible1( true  ) );
    BOOST_CHECK( bool_convertible1( false ) );

    BOOST_CHECK( bool_convertible2( 1 ) );
    BOOST_CHECK( bool_convertible2( 0 ) );

    BOOST_CHECK( bool_convertible3( (void*)1 ) );
    BOOST_CHECK( bool_convertible3( NULL ) );

    BOOST_ERROR( "Some error" );

    BOOST_REQUIRE( i == 4 );

    return 0;
}

//____________________________________________________________________________//

// EOF
