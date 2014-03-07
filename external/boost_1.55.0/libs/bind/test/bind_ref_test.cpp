//
//  bind_ref_test.cpp - reference_wrapper
//
//  Copyright (c) 2009 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/bind.hpp>
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
    X x;

    BOOST_TEST( boost::bind( &X::f, _1, 1 )( boost::ref( x ) ) == 1 );
    BOOST_TEST( boost::bind( &X::g, _1, 2 )( boost::cref( x ) ) == -2 );

    return boost::report_errors();
}
