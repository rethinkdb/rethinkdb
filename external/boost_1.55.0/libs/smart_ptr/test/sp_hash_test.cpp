//
// sp_hash_test.cpp
//
// Copyright 2011 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/shared_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
    boost::hash< boost::shared_ptr<int> > hasher;

    boost::shared_ptr< int > p1, p2( p1 ), p3( new int ), p4( p3 ), p5( new int );

    BOOST_TEST_EQ( p1, p2 );
    BOOST_TEST_EQ( hasher( p1 ), hasher( p2 ) );

    BOOST_TEST_NE( p1, p3 );
    BOOST_TEST_NE( hasher( p1 ), hasher( p3 ) );

    BOOST_TEST_EQ( p3, p4 );
    BOOST_TEST_EQ( hasher( p3 ), hasher( p4 ) );

    BOOST_TEST_NE( p3, p5 );
    BOOST_TEST_NE( hasher( p3 ), hasher( p5 ) );

    return boost::report_errors();
}
