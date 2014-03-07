//
//  weak_ptr_move_test.cpp
//
//  Copyright (c) 2007 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/weak_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <utility>

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

struct X
{
    static long instances;

    X()
    {
        ++instances;
    }

    ~X()
    {
        --instances;
    }

private:

    X( X const & );
    X & operator=( X const & );
};

long X::instances = 0;

int main()
{
    BOOST_TEST( X::instances == 0 );

    {
        boost::shared_ptr<X> p_( new X );
        boost::weak_ptr<X> p( p_ );
        BOOST_TEST( X::instances == 1 );
        BOOST_TEST( p.use_count() == 1 );

        boost::weak_ptr<X> p2( std::move( p ) );
        BOOST_TEST( X::instances == 1 );
        BOOST_TEST( p2.use_count() == 1 );
        BOOST_TEST( p.expired() );

        boost::weak_ptr<void> p3( std::move( p2 ) );
        BOOST_TEST( X::instances == 1 );
        BOOST_TEST( p3.use_count() == 1 );
        BOOST_TEST( p2.expired() );

        p_.reset();
        BOOST_TEST( X::instances == 0 );
        BOOST_TEST( p3.expired() );
    }

    {
        boost::shared_ptr<X> p_( new X );
        boost::weak_ptr<X> p( p_ );
        BOOST_TEST( X::instances == 1 );
        BOOST_TEST( p.use_count() == 1 );

        boost::weak_ptr<X> p2;
        p2 = static_cast< boost::weak_ptr<X> && >( p );
        BOOST_TEST( X::instances == 1 );
        BOOST_TEST( p2.use_count() == 1 );
        BOOST_TEST( p.expired() );

        boost::weak_ptr<void> p3;
        p3 = std::move( p2 );
        BOOST_TEST( X::instances == 1 );
        BOOST_TEST( p3.use_count() == 1 );
        BOOST_TEST( p2.expired() );

        p_.reset();
        BOOST_TEST( X::instances == 0 );
        BOOST_TEST( p3.expired() );
    }

    {
        boost::shared_ptr<X> p_( new X );
        boost::weak_ptr<X> p( p_ );
        BOOST_TEST( X::instances == 1 );
        BOOST_TEST( p.use_count() == 1 );

        boost::shared_ptr<X> p_2( new X );
        boost::weak_ptr<X> p2( p_2 );
        BOOST_TEST( X::instances == 2 );
        p2 = std::move( p );
        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( p2.use_count() == 1 );
        BOOST_TEST( p.expired() );
        BOOST_TEST( p2.lock() != p_2 );

        boost::shared_ptr<void> p_3( new X );
        boost::weak_ptr<void> p3( p_3 );
        BOOST_TEST( X::instances == 3 );
        p3 = std::move( p2 );
        BOOST_TEST( X::instances == 3 );
        BOOST_TEST( p3.use_count() == 1 );
        BOOST_TEST( p2.expired() );
        BOOST_TEST( p3.lock() != p_3 );
    }

    return boost::report_errors();
}

#else // defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

int main()
{
    return 0;
}

#endif
