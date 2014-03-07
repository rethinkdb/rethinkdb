//
//  sp_zero_compare_test.cpp - == 0, != 0
//
//  Copyright 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>

struct W
{
};

void intrusive_ptr_add_ref( W* )
{
}

void intrusive_ptr_release( W* )
{
}

int main()
{
    {
        boost::scoped_ptr<int> p;

        BOOST_TEST( p == 0 );
        BOOST_TEST( 0 == p );
        BOOST_TEST( !( p != 0 ) );
        BOOST_TEST( !( 0 != p ) );
    }

    {
        boost::scoped_array<int> p;

        BOOST_TEST( p == 0 );
        BOOST_TEST( 0 == p );
        BOOST_TEST( !( p != 0 ) );
        BOOST_TEST( !( 0 != p ) );
    }

    {
        boost::shared_ptr<int> p;

        BOOST_TEST( p == 0 );
        BOOST_TEST( 0 == p );
        BOOST_TEST( !( p != 0 ) );
        BOOST_TEST( !( 0 != p ) );
    }

    {
        boost::shared_array<int> p;

        BOOST_TEST( p == 0 );
        BOOST_TEST( 0 == p );
        BOOST_TEST( !( p != 0 ) );
        BOOST_TEST( !( 0 != p ) );
    }

    {
        boost::intrusive_ptr<W> p;

        BOOST_TEST( p == 0 );
        BOOST_TEST( 0 == p );
        BOOST_TEST( !( p != 0 ) );
        BOOST_TEST( !( 0 != p ) );
    }

    return boost::report_errors();
}
