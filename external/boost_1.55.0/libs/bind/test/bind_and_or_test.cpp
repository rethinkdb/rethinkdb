#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_and_or_test.cpp - &&, || operators
//
//  Copyright (c) 2008 Peter Dimov
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

bool f( bool x )
{
    return x;
}

bool g( bool x )
{
    return !x;
}

bool h()
{
    BOOST_ERROR( "Short-circuit evaluation failure" );
    return false;
}

template< class F, class A1, class A2, class R > void test( F f, A1 a1, A2 a2, R r )
{
    BOOST_TEST( f( a1, a2 ) == r );
}

int main()
{
    // &&

    test( boost::bind( f, true ) && boost::bind( g, true ), false, false, f( true ) && g( true ) );
    test( boost::bind( f, true ) && boost::bind( g, false ), false, false, f( true ) && g( false ) );

    test( boost::bind( f, false ) && boost::bind( h ), false, false, f( false ) && h() );

    test( boost::bind( f, _1 ) && boost::bind( g, _2 ), true, true, f( true ) && g( true ) );
    test( boost::bind( f, _1 ) && boost::bind( g, _2 ), true, false, f( true ) && g( false ) );

    test( boost::bind( f, _1 ) && boost::bind( h ), false, false, f( false ) && h() );

    // ||

    test( boost::bind( f, false ) || boost::bind( g, true ), false, false, f( false ) || g( true ) );
    test( boost::bind( f, false ) || boost::bind( g, false ), false, false, f( false ) || g( false ) );

    test( boost::bind( f, true ) || boost::bind( h ), false, false, f( true ) || h() );

    test( boost::bind( f, _1 ) || boost::bind( g, _2 ), false, true, f( false ) || g( true ) );
    test( boost::bind( f, _1 ) || boost::bind( g, _2 ), false, false, f( false ) || g( false ) );

    test( boost::bind( f, _1 ) || boost::bind( h ), true, false, f( true ) || h() );

    //

    return boost::report_errors();
}
