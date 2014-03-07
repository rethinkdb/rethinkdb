//
//  esft_void_test.cpp
//
//  Copyright 2009 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>

//

class X: public boost::enable_shared_from_this<X>
{
};

int main()
{
    boost::shared_ptr< void const volatile > pv( new X );
    boost::shared_ptr< void > pv2 = boost::const_pointer_cast< void >( pv );
    boost::shared_ptr< X > px = boost::static_pointer_cast< X >( pv2 );

    try
    {
        boost::shared_ptr< X > qx = px->shared_from_this();

        BOOST_TEST( px == qx );
        BOOST_TEST( !( px < qx ) && !( qx < px ) );
    }
    catch( boost::bad_weak_ptr const& )
    {
        BOOST_ERROR( "px->shared_from_this() failed" );
    }

    return boost::report_errors();
}
