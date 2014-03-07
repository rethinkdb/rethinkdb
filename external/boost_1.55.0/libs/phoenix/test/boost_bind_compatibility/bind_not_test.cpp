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
#include <boost/phoenix/operator.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

template<class F, class A1, class R> void tester( F f, A1 a1, R r )
{
    BOOST_TEST( f(a1) == r );
}

bool f( bool v )
{
    return v;
}

int g( int v )
{
    return v;
}

int main()
{
    using boost::phoenix::bind;
    using boost::phoenix::placeholders::_1;

    tester( !bind( f, true ), 0, !f( true ) );
    tester( !bind( g, _1 ), 5, !g( 5 ) );
    tester( bind( f, !bind( f, true ) ), 0, f( !f( true ) ) );
    tester( bind( f, !bind( f, _1 ) ), true, f( !f( true ) ) );

    return boost::report_errors();
}
