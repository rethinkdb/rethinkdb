//
//  sp_nullptr_test.cpp
//
//  Copyright 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/shared_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <cstddef>
#include <memory>

#if !defined( BOOST_NO_CXX11_NULLPTR )

struct X
{
    static int instances;

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

int X::instances = 0;

void f( std::nullptr_t )
{
}

int main()
{
    {
        boost::shared_ptr<void> p( nullptr );

        BOOST_TEST( p.get() == 0 );
        BOOST_TEST( p.use_count() == 0 );

        BOOST_TEST( p == nullptr );
        BOOST_TEST( nullptr == p );
        BOOST_TEST( !( p != nullptr ) );
        BOOST_TEST( !( nullptr != p ) );
    }

    {
        boost::shared_ptr<int> p( nullptr, f );

        BOOST_TEST( p.get() == 0 );
        BOOST_TEST( p.use_count() == 1 );

        BOOST_TEST( p == nullptr );
        BOOST_TEST( nullptr == p );
        BOOST_TEST( !( p != nullptr ) );
        BOOST_TEST( !( nullptr != p ) );
    }

    {
        boost::shared_ptr<int> p( nullptr, f, std::allocator<int>() );

        BOOST_TEST( p.get() == 0 );
        BOOST_TEST( p.use_count() == 1 );

        BOOST_TEST( p == nullptr );
        BOOST_TEST( nullptr == p );
        BOOST_TEST( !( p != nullptr ) );
        BOOST_TEST( !( nullptr != p ) );
    }

    {
        boost::shared_ptr<int> p( new int );

        BOOST_TEST( p.get() != 0 );
        BOOST_TEST( p.use_count() == 1 );

        BOOST_TEST( p != nullptr );
        BOOST_TEST( nullptr != p );
        BOOST_TEST( !( p == nullptr ) );
        BOOST_TEST( !( nullptr == p ) );

        p = nullptr;

        BOOST_TEST( p.get() == 0 );
        BOOST_TEST( p.use_count() == 0 );

        BOOST_TEST( p == nullptr );
        BOOST_TEST( nullptr == p );
        BOOST_TEST( !( p != nullptr ) );
        BOOST_TEST( !( nullptr != p ) );
    }

    {
        BOOST_TEST( X::instances == 0 );

        boost::shared_ptr<X> p( new X );
        BOOST_TEST( X::instances == 1 );

        BOOST_TEST( p.get() != 0 );
        BOOST_TEST( p.use_count() == 1 );

        BOOST_TEST( p != nullptr );
        BOOST_TEST( nullptr != p );
        BOOST_TEST( !( p == nullptr ) );
        BOOST_TEST( !( nullptr == p ) );

        p = nullptr;
        BOOST_TEST( X::instances == 0 );

        BOOST_TEST( p.get() == 0 );
        BOOST_TEST( p.use_count() == 0 );

        BOOST_TEST( p == nullptr );
        BOOST_TEST( nullptr == p );
        BOOST_TEST( !( p != nullptr ) );
        BOOST_TEST( !( nullptr != p ) );
    }

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
