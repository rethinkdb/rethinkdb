// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//


#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // suppress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/range/rbegin.hpp>
#include <boost/range/rend.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>
#include <algorithm>

void check_iterator()
{
    typedef std::vector<int>                            vec_t;
    typedef vec_t::iterator                             iterator;
    typedef std::pair<iterator,iterator>                pair_t;
    typedef boost::range_reverse_iterator<pair_t>::type rev_iterator;
    typedef std::pair<rev_iterator,rev_iterator>        rev_pair_t;

    vec_t                            vec;
    pair_t                           p    = std::make_pair( vec.begin(), vec.end() );
    rev_pair_t                       rp   = std::make_pair( boost::rbegin( p ), boost::rend( p ) );
    int                             a[]  = {1,2,3,4,5,6,7,8,9,10};
    const int                       ca[] = {1,2,3,4,5,6,7,8,9,10,11,12};
    BOOST_CHECK( boost::rbegin( vec ) == boost::range_reverse_iterator<vec_t>::type( vec.end() ) );
    BOOST_CHECK( boost::rend( vec ) == boost::range_reverse_iterator<vec_t>::type( vec.begin() ) );
    BOOST_CHECK( std::distance( boost::rbegin( vec ), boost::rend( vec ) ) == std::distance( boost::begin( vec ), boost::end( vec ) ) );

    BOOST_CHECK( boost::rbegin( p ) == boost::begin( rp ) );
    BOOST_CHECK( boost::rend( p ) == boost::end( rp ) );
    BOOST_CHECK( std::distance( boost::rbegin( p ), boost::rend( p ) ) == std::distance( boost::begin( rp ), boost::end( rp ) ) );
    BOOST_CHECK( std::distance( boost::begin( p ), boost::end( p ) ) == std::distance( boost::rbegin( rp ), boost::rend( rp ) ) );


    BOOST_CHECK_EQUAL( &*boost::begin( a ), &*( boost::rend( a ) - 1 ) );
    BOOST_CHECK_EQUAL( &*( boost::end( a ) - 1 ), &*boost::rbegin( a ) );
    BOOST_CHECK_EQUAL( &*boost::begin( ca ), &*( boost::rend( ca ) - 1 ) );
    BOOST_CHECK_EQUAL( &*( boost::end( ca ) - 1 ), &*boost::rbegin( ca ) );
}


boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add( BOOST_TEST_CASE( &check_iterator ) );

    return test;
}







