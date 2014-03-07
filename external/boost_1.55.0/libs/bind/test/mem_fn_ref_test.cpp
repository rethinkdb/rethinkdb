//
//  mem_fn_ref_test.cpp - reference_wrapper
//
//  Copyright (c) 2009 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/mem_fn.hpp>
#include <boost/ref.hpp>
#include <boost/detail/lightweight_test.hpp>

struct X
{
    int f()
    {
        return 1;
    }

    int g() const
    {
        return 2;
    }
};

int main()
{
    X x;

    BOOST_TEST( boost::mem_fn( &X::f )( boost::ref( x ) ) == 1 );
    BOOST_TEST( boost::mem_fn( &X::g )( boost::cref( x ) ) == 2 );

    return boost::report_errors();
}
