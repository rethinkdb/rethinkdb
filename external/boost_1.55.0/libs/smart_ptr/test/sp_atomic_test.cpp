#include <boost/config.hpp>

//  sp_atomic_test.cpp
//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt


#include <boost/detail/lightweight_test.hpp>
#include <boost/shared_ptr.hpp>

//

struct X
{
};

#define BOOST_TEST_SP_EQ( p, q ) BOOST_TEST( p == q && !( p < q ) && !( q < p ) )

int main()
{
    boost::shared_ptr<X> px( new X );

    {
        boost::shared_ptr<X> p2 = boost::atomic_load( &px );
        BOOST_TEST_SP_EQ( p2, px );

        boost::shared_ptr<X> px2( new X );
        boost::atomic_store( &px, px2 );
        BOOST_TEST_SP_EQ( px, px2 );

        p2 = boost::atomic_load( &px );
        BOOST_TEST_SP_EQ( p2, px );
        BOOST_TEST_SP_EQ( p2, px2 );

        boost::shared_ptr<X> px3( new X );
        boost::shared_ptr<X> p3 = boost::atomic_exchange( &px, px3 );
        BOOST_TEST_SP_EQ( p3, px2 );
        BOOST_TEST_SP_EQ( px, px3 );

        boost::shared_ptr<X> px4( new X );
        boost::shared_ptr<X> cmp;

        bool r = boost::atomic_compare_exchange( &px, &cmp, px4 );
        BOOST_TEST( !r );
        BOOST_TEST_SP_EQ( px, px3 );
        BOOST_TEST_SP_EQ( cmp, px3 );

        r = boost::atomic_compare_exchange( &px, &cmp, px4 );
        BOOST_TEST( r );
        BOOST_TEST_SP_EQ( px, px4 );
    }

    //

    px.reset();

    {
        boost::shared_ptr<X> p2 = boost::atomic_load_explicit( &px, boost::memory_order_acquire );
        BOOST_TEST_SP_EQ( p2, px );

        boost::shared_ptr<X> px2( new X );
        boost::atomic_store_explicit( &px, px2, boost::memory_order_release );
        BOOST_TEST_SP_EQ( px, px2 );

        boost::shared_ptr<X> p3 = boost::atomic_exchange_explicit( &px, boost::shared_ptr<X>(), boost::memory_order_acq_rel );
        BOOST_TEST_SP_EQ( p3, px2 );
        BOOST_TEST_SP_EQ( px, p2 );

        boost::shared_ptr<X> px4( new X );
        boost::shared_ptr<X> cmp( px2 );

        bool r = boost::atomic_compare_exchange_explicit( &px, &cmp, px4, boost::memory_order_acquire, boost::memory_order_relaxed );
        BOOST_TEST( !r );
        BOOST_TEST_SP_EQ( px, p2 );
        BOOST_TEST_SP_EQ( cmp, p2 );

        r = boost::atomic_compare_exchange_explicit( &px, &cmp, px4, boost::memory_order_release, boost::memory_order_acquire );
        BOOST_TEST( r );
        BOOST_TEST_SP_EQ( px, px4 );
    }

    return boost::report_errors();
}
