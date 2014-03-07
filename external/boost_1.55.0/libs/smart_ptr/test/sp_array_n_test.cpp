//
//  sp_array_n_test.cpp
//
//  Copyright (c) 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <memory>
#include <utility>

class X: public boost::enable_shared_from_this< X >
{
public:

    static int allocations;
    static int instances;

    X()
    {
        ++instances;
    }

    ~X()
    {
        --instances;
    }

    void* operator new[]( std::size_t n )
    {
        ++allocations;
        return ::operator new[]( n );
    }

    void operator delete[]( void* p )
    {
        --allocations;
        ::operator delete[]( p );
    }

private:

    X( X const& );
    X& operator=( X const& );
};

int X::allocations = 0;
int X::instances = 0;

template< class T> class array_deleter
{
public:

    static int calls;

    void operator()( T * p ) const
    {
        ++calls;
        delete[] p;
    }

private:

    template< class Y > void operator()( Y * p ) const;
};

template< class T > int array_deleter< T >::calls = 0;

int main()
{
    BOOST_TEST( X::allocations == 0 );
    BOOST_TEST( X::instances == 0 );

    {
        boost::shared_ptr<X[3]> px;
        BOOST_TEST( !px );

        BOOST_TEST( X::allocations == 0 );
        BOOST_TEST( X::instances == 0 );

        boost::shared_ptr<X[3]> px2( new X[ 3 ] );
        BOOST_TEST( px2 );

        try
        {
            px2[0].shared_from_this();
            BOOST_ERROR( "px2[0].shared_from_this() failed to throw" );
        }
        catch( boost::bad_weak_ptr const& )
        {
        }
        catch( ... )
        {
            BOOST_ERROR( "px2[0].shared_from_this() threw something else than bad_weak_ptr" );
        }

        BOOST_TEST( X::allocations == 1 );
        BOOST_TEST( X::instances == 3 );

        {
            X & rx = px2[ 0 ];
            BOOST_TEST( &rx == px2.get() );
        }

        boost::shared_ptr<X const[3]> px3( px2 );
        BOOST_TEST( px3 == px2 );
        BOOST_TEST( !( px2 < px3 ) && !( px3 < px2 ) );

        {
            X const & rx = px3[ 1 ];
            BOOST_TEST( &rx == px3.get() + 1 );
        }

        px3.reset();
        px3 = px2;
        BOOST_TEST( px3 == px2 );
        BOOST_TEST( !( px2 < px3 ) && !( px3 < px2 ) );

        boost::shared_ptr<X volatile[3]> px4( px2 );
        BOOST_TEST( px4 == px2 );
        BOOST_TEST( !( px2 < px4 ) && !( px4 < px2 ) );

        {
            X volatile & rx = px4[ 2 ];
            BOOST_TEST( &rx == px4.get() + 2 );
        }

        px4.reset();
        px4 = px2;
        BOOST_TEST( px4 == px2 );
        BOOST_TEST( !( px2 < px4 ) && !( px4 < px2 ) );

        boost::shared_ptr<void> px5( px2 );
        BOOST_TEST( px5 == px2 );
        BOOST_TEST( !( px2 < px5 ) && !( px5 < px2 ) );

        px5.reset();
        px5 = px2;
        BOOST_TEST( px5 == px2 );
        BOOST_TEST( !( px2 < px5 ) && !( px5 < px2 ) );

        boost::weak_ptr<X[3]> wp( px );
        BOOST_TEST( wp.lock() == px );

        boost::weak_ptr<X[3]> wp2( px2 );
        BOOST_TEST( wp2.lock() == px2 );

        wp2.reset();
        wp2 = px2;
        BOOST_TEST( wp2.lock() == px2 );

        boost::weak_ptr<X const[3]> wp3( px2 );
        BOOST_TEST( wp3.lock() == px2 );

        wp3.reset();
        wp3 = px2;
        BOOST_TEST( wp3.lock() == px2 );

        boost::weak_ptr<X volatile[3]> wp4( px2 );
        BOOST_TEST( wp4.lock() == px2 );

        wp4.reset();
        wp4 = px2;
        BOOST_TEST( wp4.lock() == px2 );

        boost::weak_ptr<void> wp5( px2 );
        BOOST_TEST( wp5.lock() == px2 );

        wp5.reset();
        wp5 = px2;
        BOOST_TEST( wp5.lock() == px2 );

        px2.reset();

        BOOST_TEST( X::allocations == 1 );
        BOOST_TEST( X::instances == 3 );

        px3.reset();
        px4.reset();
        px5.reset();

        BOOST_TEST( X::allocations == 0 );
        BOOST_TEST( X::instances == 0 );

        BOOST_TEST( wp2.lock() == 0 );
        BOOST_TEST( wp3.lock() == 0 );
        BOOST_TEST( wp4.lock() == 0 );
        BOOST_TEST( wp5.lock() == 0 );
    }

    {
        boost::shared_ptr<X[5]> px( new X[ 5 ], array_deleter< X >() );
        BOOST_TEST( X::allocations == 1 );
        BOOST_TEST( X::instances == 5 );

        try
        {
            px[0].shared_from_this();
            BOOST_ERROR( "px[0].shared_from_this() failed to throw" );
        }
        catch( boost::bad_weak_ptr const& )
        {
        }
        catch( ... )
        {
            BOOST_ERROR( "px[0].shared_from_this() threw something else than bad_weak_ptr" );
        }

        px.reset();

        BOOST_TEST( X::allocations == 0 );
        BOOST_TEST( X::instances == 0 );
        BOOST_TEST( array_deleter< X >::calls == 1 );
    }

    {
        boost::shared_ptr<X[6]> px( new X[ 6 ], array_deleter< X >(), std::allocator< X >() );
        BOOST_TEST( X::allocations == 1 );
        BOOST_TEST( X::instances == 6 );

        try
        {
            px[0].shared_from_this();
            BOOST_ERROR( "px[0].shared_from_this() failed to throw" );
        }
        catch( boost::bad_weak_ptr const& )
        {
        }
        catch( ... )
        {
            BOOST_ERROR( "px[0].shared_from_this() threw something else than bad_weak_ptr" );
        }

        px.reset();

        BOOST_TEST( X::allocations == 0 );
        BOOST_TEST( X::instances == 0 );
        BOOST_TEST( array_deleter< X >::calls == 2 );
    }

    return boost::report_errors();
}
