// rvalue_test - test lambda function objects with rvalue arguments
//
// Copyright (c) 2007 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/lambda/lambda.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
    using namespace boost::lambda;

    int x = 0;
    int const y = 1;
    int const z = 2;

    BOOST_TEST( _1( x ) == 0 );
    BOOST_TEST( _1( y ) == 1 );
    BOOST_TEST( _1( 2 ) == 2 );

    BOOST_TEST( _2( x, x ) == 0 );
    BOOST_TEST( _2( x, y ) == 1 );
    BOOST_TEST( _2( x, 2 ) == 2 );

    BOOST_TEST( _2( 4, x ) == 0 );
    BOOST_TEST( _2( 4, y ) == 1 );
    BOOST_TEST( _2( 4, 2 ) == 2 );

    (_1 = _2)( x, y );
    BOOST_TEST( x == y );

    (_1 = _2)( x, 3 );
    BOOST_TEST( x == 3 );

    (_2 = _1)( z, x );
    BOOST_TEST( x == z );

    (_2 = _1)( 4, x );
    BOOST_TEST( x == 4 );

    BOOST_TEST( _3( x, x, x ) == x );
    BOOST_TEST( _3( x, x, y ) == y );
    BOOST_TEST( _3( x, x, 2 ) == 2 );

    BOOST_TEST( _3( x, 5, x ) == x );
    BOOST_TEST( _3( x, 5, y ) == y );
    BOOST_TEST( _3( x, 5, 2 ) == 2 );

    BOOST_TEST( _3( 9, 5, x ) == x );
    BOOST_TEST( _3( 9, 5, y ) == y );
    BOOST_TEST( _3( 9, 5, 2 ) == 2 );

    return boost::report_errors();
}
