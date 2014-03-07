#include <boost/config.hpp>

//  shared_ptr_alias_test.cpp
//
//  Copyright (c) 2007 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <boost/detail/lightweight_test.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <cstddef>

//

class incomplete;

struct X
{
    int v_;

    explicit X( int v ): v_( v )
    {
    }

    ~X()
    {
        v_ = 0;
    }
};

int main()
{
    {
        int m = 0;
        boost::shared_ptr< int > p;
        boost::shared_ptr< int > p2( p, &m );

        BOOST_TEST( p2.get() == &m );
        BOOST_TEST( p2? true: false );
        BOOST_TEST( !!p2 );
        BOOST_TEST( p2.use_count() == p.use_count() );
        BOOST_TEST( !( p < p2 ) && !( p2 < p ) );

        p2.reset( p, 0 );

        BOOST_TEST( p2.get() == 0 );
        BOOST_TEST( p2? false: true );
        BOOST_TEST( !p2 );
        BOOST_TEST( p2.use_count() == p.use_count() );
        BOOST_TEST( !( p < p2 ) && !( p2 < p ) );
    }

    {
        int m = 0;
        boost::shared_ptr< int > p( new int );
        boost::shared_ptr< int const > p2( p, &m );

        BOOST_TEST( p2.get() == &m );
        BOOST_TEST( p2? true: false );
        BOOST_TEST( !!p2 );
        BOOST_TEST( p2.use_count() == p.use_count() );
        BOOST_TEST( !( p < p2 ) && !( p2 < p ) );

        boost::shared_ptr< int volatile > p3;
        p2.reset( p3, 0 );

        BOOST_TEST( p2.get() == 0 );
        BOOST_TEST( p2? false: true );
        BOOST_TEST( !p2 );
        BOOST_TEST( p2.use_count() == p3.use_count() );
        BOOST_TEST( !( p3 < p2 ) && !( p2 < p3 ) );
    }

    {
        boost::shared_ptr< int > p( new int );
        boost::shared_ptr< void const > p2( p, 0 );

        BOOST_TEST( p2.get() == 0 );
        BOOST_TEST( p2? false: true );
        BOOST_TEST( !p2 );
        BOOST_TEST( p2.use_count() == p.use_count() );
        BOOST_TEST( !( p < p2 ) && !( p2 < p ) );

        int m = 0;
        boost::shared_ptr< void volatile > p3;

        p2.reset( p3, &m );

        BOOST_TEST( p2.get() == &m );
        BOOST_TEST( p2? true: false );
        BOOST_TEST( !!p2 );
        BOOST_TEST( p2.use_count() == p3.use_count() );
        BOOST_TEST( !( p3 < p2 ) && !( p2 < p3 ) );
    }

    {
        boost::shared_ptr< incomplete > p;
        boost::shared_ptr< incomplete > p2( p, 0 );

        BOOST_TEST( p2.get() == 0 );
        BOOST_TEST( p2? false: true );
        BOOST_TEST( !p2 );
        BOOST_TEST( p2.use_count() == p.use_count() );
        BOOST_TEST( !( p < p2 ) && !( p2 < p ) );

        p2.reset( p, 0 );

        BOOST_TEST( p2.get() == 0 );
        BOOST_TEST( p2? false: true );
        BOOST_TEST( !p2 );
        BOOST_TEST( p2.use_count() == p.use_count() );
        BOOST_TEST( !( p < p2 ) && !( p2 < p ) );
    }

    {
        boost::shared_ptr< X > p( new X( 5 ) );
        boost::shared_ptr< int const > p2( p, &p->v_ );

        BOOST_TEST( p2.get() == &p->v_ );
        BOOST_TEST( p2? true: false );
        BOOST_TEST( !!p2 );
        BOOST_TEST( p2.use_count() == p.use_count() );
        BOOST_TEST( !( p < p2 ) && !( p2 < p ) );

        p.reset();
        BOOST_TEST( *p2 == 5 );

        boost::shared_ptr< X const > p3( new X( 8 ) );
        p2.reset( p3, &p3->v_ );

        BOOST_TEST( p2.get() == &p3->v_ );
        BOOST_TEST( p2? true: false );
        BOOST_TEST( !!p2 );
        BOOST_TEST( p2.use_count() == p3.use_count() );
        BOOST_TEST( !( p3 < p2 ) && !( p2 < p3 ) );

        p3.reset();
        BOOST_TEST( *p2 == 8 );
    }

    return boost::report_errors();
}
