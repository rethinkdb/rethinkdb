#include <boost/config.hpp>

#if defined( BOOST_MSVC )

#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed

#endif

//  bind_rvalue_test.cpp
//
//  Copyright (c) 2006 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/bind.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

//

int f( int x )
{
    return x;
}

int main()
{
    BOOST_TEST( 
        boost::bind( f, _1 )
        ( 1 ) == 1 );

    BOOST_TEST( 
        boost::bind( f, _2 )
        ( 1, 2 ) == 2 );

    BOOST_TEST( 
        boost::bind( f, _3 )
        ( 1, 2, 3 ) == 3 );

    BOOST_TEST( 
        boost::bind( f, _4 )
        ( 1, 2, 3, 4 ) == 4 );

    BOOST_TEST( 
        boost::bind( f, _5 )
        ( 1, 2, 3, 4, 5 ) == 5 );

    BOOST_TEST( 
        boost::bind( f, _6 )
        ( 1, 2, 3, 4, 5, 6 ) == 6 );

    BOOST_TEST( 
        boost::bind( f, _7 )
        ( 1, 2, 3, 4, 5, 6, 7 ) == 7 );

    BOOST_TEST( 
        boost::bind( f, _8 )
        ( 1, 2, 3, 4, 5, 6, 7, 8 ) == 8 );

    BOOST_TEST( 
        boost::bind( f, _9 )
        ( 1, 2, 3, 4, 5, 6, 7, 8, 9 ) == 9 );

    return boost::report_errors();
}
