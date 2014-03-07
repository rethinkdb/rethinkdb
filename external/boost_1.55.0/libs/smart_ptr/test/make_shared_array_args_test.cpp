// make_shared_array_args_test.cpp
//
// Copyright 2007-2009, 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/detail/lightweight_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cstddef>

class X
{
private:

    X( X const & );
    X & operator=( X const & );

    void * operator new[]( std::size_t n );
    void operator delete[]( void * p );

public:

    static int instances;

    int v;

    explicit X( int a1 = 0, int a2 = 0, int a3 = 0, int a4 = 0, int a5 = 0, int a6 = 0, int a7 = 0, int a8 = 0, int a9 = 0 ): v( a1+a2+a3+a4+a5+a6+a7+a8+a9 )
    {
        ++instances;
    }

    ~X()
    {
        --instances;
    }
};

int X::instances = 0;

int main()
{
    BOOST_TEST( X::instances == 0 );

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 0 );
        BOOST_TEST( px[1].v == 0 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

#if !defined( BOOST_NO_CXX11_VARIADIC_TEMPLATES ) && !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1 );
        BOOST_TEST( px[1].v == 1 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2 );
        BOOST_TEST( px[1].v == 1+2 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2, 3 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2+3 );
        BOOST_TEST( px[1].v == 1+2+3 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2, 3, 4 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2+3+4 );
        BOOST_TEST( px[1].v == 1+2+3+4 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2, 3, 4, 5 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2+3+4+5 );
        BOOST_TEST( px[1].v == 1+2+3+4+5 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2, 3, 4, 5, 6 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2+3+4+5+6 );
        BOOST_TEST( px[1].v == 1+2+3+4+5+6 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2, 3, 4, 5, 6, 7 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2+3+4+5+6+7 );
        BOOST_TEST( px[1].v == 1+2+3+4+5+6+7 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2, 3, 4, 5, 6, 7, 8 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2+3+4+5+6+7+8 );
        BOOST_TEST( px[1].v == 1+2+3+4+5+6+7+8 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

    {
        boost::shared_ptr< X[] > px = boost::make_shared< X[] >( 2, 1, 2, 3, 4, 5, 6, 7, 8, 9 );

        BOOST_TEST( X::instances == 2 );
        BOOST_TEST( px[0].v == 1+2+3+4+5+6+7+8+9 );
        BOOST_TEST( px[1].v == 1+2+3+4+5+6+7+8+9 );

        px.reset();

        BOOST_TEST( X::instances == 0 );
    }

#endif

    return boost::report_errors();
}
