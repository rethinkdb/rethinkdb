#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_dm_test.cpp - data members
//
//  Copyright (c) 2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/bind.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

struct X
{
    int m;
};

X f( int v )
{
    X r = { v };
    return r;
}

int main()
{
    X x = { 17041 };
    X * px = &x;

    BOOST_TEST( boost::bind( &X::m, _1 )( x ) == 17041 );
    BOOST_TEST( boost::bind( &X::m, _1 )( px ) == 17041 );

    BOOST_TEST( boost::bind( &X::m, x )() == 17041 );
    BOOST_TEST( boost::bind( &X::m, px )() == 17041 );
    BOOST_TEST( boost::bind( &X::m, boost::ref(x) )() == 17041 );


    X const cx = x;
    X const * pcx = &cx;

    BOOST_TEST( boost::bind( &X::m, _1 )( cx ) == 17041 );
    BOOST_TEST( boost::bind( &X::m, _1 )( pcx ) == 17041 );

    BOOST_TEST( boost::bind( &X::m, cx )() == 17041 );
    BOOST_TEST( boost::bind( &X::m, pcx )() == 17041 );
    BOOST_TEST( boost::bind( &X::m, boost::ref(cx) )() == 17041 );

    int const v = 42;

    BOOST_TEST( boost::bind( &X::m, boost::bind( f, _1 ) )( v ) == v );

    return boost::report_errors();
}
