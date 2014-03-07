#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_rel_test.cpp - ==, !=, <, <=, >, >= operators
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

int f( int x )
{
    return x + x;
}

int g( int x )
{
    return 2 * x;
}

int main()
{
    int x = 4;
    int y = x + x;

    // bind op value

    BOOST_TEST( ( boost::bind( f, _1 ) == y )( x ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) != y )( x ) ) );

    BOOST_TEST( !( ( boost::bind( f, _1 ) < y )( x ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) < y + 1 )( x ) );

    BOOST_TEST( !( ( boost::bind( f, _1 ) > y )( x ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) > y - 1 )( x ) );

    BOOST_TEST( !( ( boost::bind( f, _1 ) <= y - 1 )( x ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) <= y )( x ) );
    BOOST_TEST( ( boost::bind( f, _1 ) <= y + 1 )( x ) );

    BOOST_TEST( !( ( boost::bind( f, _1 ) >= y + 1 )( x ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) >= y )( x ) );
    BOOST_TEST( ( boost::bind( f, _1 ) >= y - 1 )( x ) );

    // bind op ref

    BOOST_TEST( ( boost::bind( f, _1 ) == boost::ref( y ) )( x ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) != boost::ref( y ) )( x ) ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) < boost::ref( y ) )( x ) ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) > boost::ref( y ) )( x ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) <= boost::ref( y ) )( x ) );
    BOOST_TEST( ( boost::bind( f, _1 ) >= boost::ref( y ) )( x ) );

    // bind op placeholder

    BOOST_TEST( ( boost::bind( f, _1 ) == _2 )( x, y ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) != _2 )( x, y ) ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) < _2 )( x, y ) ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) > _2 )( x, y ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) <= _2 )( x, y ) );
    BOOST_TEST( ( boost::bind( f, _1 ) >= _2 )( x, y ) );

    // bind op bind

    // important: bind( f, _1 ) and bind( g, _1 ) have the same type
    BOOST_TEST( ( boost::bind( f, _1 ) == boost::bind( g, _1 ) )( x ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) != boost::bind( g, _1 ) )( x ) ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) < boost::bind( g, _1 ) )( x ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) <= boost::bind( g, _1 ) )( x ) );
    BOOST_TEST( !( ( boost::bind( f, _1 ) > boost::bind( g, _1 ) )( x ) ) );
    BOOST_TEST( ( boost::bind( f, _1 ) >= boost::bind( g, _1 ) )( x ) );

    return boost::report_errors();
}
