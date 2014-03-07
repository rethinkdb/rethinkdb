#include <boost/config.hpp>

#if defined( BOOST_MSVC )

#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed

#endif

//  bind_placeholder_test.cpp - test custom placeholders
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

long f( long a, long b, long c, long d, long e, long f, long g, long h, long i )
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h + 100000000 * i;
}

template< int I > struct custom_placeholder
{
};

namespace boost
{

template< int I > struct is_placeholder< custom_placeholder< I > >
{
    enum { value = I };
};

} // namespace boost

int main()
{
    int const x1 = 1;
    int const x2 = 2;
    int const x3 = 3;
    int const x4 = 4;
    int const x5 = 5;
    int const x6 = 6;
    int const x7 = 7;
    int const x8 = 8;
    int const x9 = 9;

    custom_placeholder<1> p1;
    custom_placeholder<2> p2;
    custom_placeholder<3> p3;
    custom_placeholder<4> p4;
    custom_placeholder<5> p5;
    custom_placeholder<6> p6;
    custom_placeholder<7> p7;
    custom_placeholder<8> p8;
    custom_placeholder<9> p9;

    BOOST_TEST( 
        boost::bind( f, p1, p2, p3, p4, p5, p6, p7, p8, p9 )
        ( x1, x2, x3, x4, x5, x6, x7, x8, x9 ) == 987654321L );

    return boost::report_errors();
}
