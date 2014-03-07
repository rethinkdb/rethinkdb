/*==============================================================================
    Copyright (c) 2009 Peter Dimov
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/ref.hpp>
#include <boost/detail/lightweight_test.hpp>

struct X
{
    int f( int x )
    {
        return x;
    }

    int g( int x ) const
    {
        return -x;
    }
};

int main()
{
    using boost::phoenix::bind;
    using boost::phoenix::placeholders::_1;
    X x;

    BOOST_TEST( bind( &X::f, _1, 1 )( boost::ref( x ) ) == 1 );
    BOOST_TEST( bind( &X::g, _1, 2 )( boost::cref( x ) ) == -2 );

    return boost::report_errors();
}
