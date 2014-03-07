#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  mem_fn_dm_test.cpp - data members
//
//  Copyright (c) 2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/mem_fn.hpp>

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

int main()
{
    X x = { 0 };

    boost::mem_fn( &X::m )( x ) = 401;

    BOOST_TEST( x.m == 401 );
    BOOST_TEST( boost::mem_fn( &X::m )( x ) == 401 );

    boost::mem_fn( &X::m )( &x ) = 502;

    BOOST_TEST( x.m == 502 );
    BOOST_TEST( boost::mem_fn( &X::m )( &x ) == 502 );

    X * px = &x;

    boost::mem_fn( &X::m )( px ) = 603;

    BOOST_TEST( x.m == 603 );
    BOOST_TEST( boost::mem_fn( &X::m )( px ) == 603 );

    X const & cx = x;
    X const * pcx = &x;

    BOOST_TEST( boost::mem_fn( &X::m )( cx ) == 603 );
    BOOST_TEST( boost::mem_fn( &X::m )( pcx ) == 603 );

    return boost::report_errors();
}
