//
//  esft_second_ptr_test.cpp
//
//  This test has been extracted from a real
//  scenario that occurs in Boost.Python
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

void null_deleter( void const* )
{
}

int main()
{
    boost::shared_ptr<X> px( new X );

    {
        boost::shared_ptr<X> px2( px.get(), null_deleter );
        BOOST_TEST( px == px2 );
    }

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
