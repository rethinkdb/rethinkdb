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
#include <boost/detail/lightweight_test.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>
#include <boost/function.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

int f( int x )
{
    return x;
}

int g( int x )
{
    return x + 1;
}

int main()
{
    using boost::phoenix::bind;

    boost::function0<int> fn;

    BOOST_TEST( !fn.contains( bind( f, 1 ) ) );
    BOOST_TEST( !fn.contains( bind( f, 2 ) ) );
    BOOST_TEST( !fn.contains( bind( g, 1 ) ) );

    fn = bind( f, 1 );

    BOOST_TEST( fn() == 1 );

    BOOST_TEST( fn.contains( bind( f, 1 ) ) );
    BOOST_TEST( !fn.contains( bind( f, 2 ) ) );
    BOOST_TEST( !fn.contains( bind( g, 1 ) ) );

    fn = bind( f, 2 );

    BOOST_TEST( fn() == 2 );

    BOOST_TEST( !fn.contains( bind( f, 1 ) ) );
    BOOST_TEST( fn.contains( bind( f, 2 ) ) );
    BOOST_TEST( !fn.contains( bind( g, 1 ) ) );

    fn = bind( g, 1 );

    BOOST_TEST( fn() == 2 );

    BOOST_TEST( !fn.contains( bind( f, 1 ) ) );
    BOOST_TEST( !fn.contains( bind( f, 2 ) ) );
    BOOST_TEST( fn.contains( bind( g, 1 ) ) );

    return boost::report_errors();
}
