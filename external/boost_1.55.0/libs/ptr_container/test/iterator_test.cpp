//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//
 
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/test/test_tools.hpp>

void test_iterator()
{
    using namespace boost;
    ptr_vector<int> vec;
    vec.push_back( new int(0) );
    ptr_vector<int>::iterator           mutable_i   = vec.begin();
    ptr_vector<int>::const_iterator     const_i     = vec.begin();

    BOOST_CHECK( mutable_i == const_i );
    BOOST_CHECK( ! (mutable_i != const_i ) );
    BOOST_CHECK( const_i == mutable_i );
    BOOST_CHECK( ! ( const_i != mutable_i ) );

    BOOST_CHECK( !( mutable_i < const_i ) );
    BOOST_CHECK( mutable_i <= const_i );
    BOOST_CHECK( ! ( mutable_i > const_i ) );
    BOOST_CHECK( mutable_i >= const_i  );
    BOOST_CHECK( !( const_i < mutable_i ) );
    BOOST_CHECK( const_i <= mutable_i );
    BOOST_CHECK( ! ( const_i > mutable_i ) );
    BOOST_CHECK( const_i >= mutable_i );
    
    BOOST_CHECK( const_i - mutable_i == 0 );
    BOOST_CHECK( mutable_i - const_i == 0 );
    
    const ptr_vector<int>& rvec               = vec;
    const_i                                   = rvec.begin();

    ptr_map<int,int> map;
    int i = 0;
    map.insert( i, new int(0) );
    ptr_map<int,int>::iterator map_mutable_i     = map.begin();
    ptr_map<int,int>::const_iterator map_const_i = map.begin();

#if !BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(70190006))
    // This only works for library implementations which conform to the
    // proposed resolution of the C++ Standard Library DR#179. See
    // http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#179.
    BOOST_CHECK( map_mutable_i == map_const_i );
    BOOST_CHECK( ! ( map_mutable_i != map_const_i ) );
    BOOST_CHECK( map_const_i == map_mutable_i );
    BOOST_CHECK( ! ( map_const_i != map_mutable_i ) );
#endif

    const ptr_map<int,int>& rmap = map;
    map_const_i = rmap.begin();
}

#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_iterator ) );

    return test;
}




