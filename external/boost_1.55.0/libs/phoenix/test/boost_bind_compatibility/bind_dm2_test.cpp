/*==============================================================================
    Copyright (c) 2005 Peter Dimov
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>
#include <string>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

struct X
{
    int m;
};

struct Y
{
    char m[ 64 ];
};

int main()
{
    using boost::phoenix::bind;
    using boost::phoenix::ref;
    using boost::phoenix::placeholders::_1;

    X x = { 0 };
    X * px = &x;

    bind( &X::m, _1 )( px ) = 42;

    BOOST_TEST( x.m == 42 );

    bind( &X::m, ref(x) )() = 17041;

    BOOST_TEST( x.m == 17041 );

    X const * pcx = &x;

    BOOST_TEST( bind( &X::m, _1 )( pcx ) == 17041L );
    BOOST_TEST( bind( &X::m, pcx )() == 17041L );

    Y y = { "test" };
    std::string v( "test" );

    BOOST_TEST( bind( &Y::m, &y )() == v );
    BOOST_TEST( bind( &Y::m, &y )() == v );

    return boost::report_errors();
}
