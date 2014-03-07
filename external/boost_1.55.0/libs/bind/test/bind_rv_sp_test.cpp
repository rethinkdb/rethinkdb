#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_rv_sp_test.cpp - smart pointer returned by value from an inner bind
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
#include <boost/shared_ptr.hpp>

struct X
{
    int v_;

    X( int v ): v_( v )
    {
    }

    int f()
    {
        return v_;
    }
};

struct Y
{
    boost::shared_ptr<X> f()
    {
        return boost::shared_ptr<X>( new X( 42 ) );
    }
};

int main()
{
    Y y;

    BOOST_TEST( boost::bind( &X::f, boost::bind( &Y::f, &y ) )() == 42 );

    return boost::report_errors();
}
