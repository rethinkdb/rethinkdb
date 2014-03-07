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

#include <boost/range.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace boost;
using namespace std;

void check_array()
{
    const int sz = 9;
    typedef int   array_t[sz];
    int           my_array[sz]  = { 1,2,3,4,5,6,7,8,9 };
    const array_t ca           = { 1,2,3,4,5,6,7,8,10 };

 
// BOOST_RANGE_NO_STATIC_ASSERT 
#if !defined( __BORLANDC__ )
#else
    BOOST_STATIC_ASSERT(( is_same< range_value<array_t>::type, int >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_iterator<array_t>::type, int* >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_const_iterator<array_t>::type, const int* >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_difference<array_t>::type, std::ptrdiff_t >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_size<array_t>::type, std::size_t >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_iterator<array_t>::type, int* >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_iterator<const array_t>::type, const int* >::value ));
    
    BOOST_STATIC_ASSERT(( is_same< range_value<const array_t>::type, const int >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_iterator<const array_t>::type, const int* >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_const_iterator<const array_t>::type, const int* >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_difference<const array_t>::type, std::ptrdiff_t >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_size<const array_t>::type, std::size_t >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_iterator<const array_t>::type, const int* >::value ));
    BOOST_STATIC_ASSERT(( is_same< range_iterator<const array_t>::type, const int* >::value ));
#endif
    
    BOOST_CHECK_EQUAL( begin( my_array ), my_array );
    BOOST_CHECK_EQUAL( end( my_array ), my_array + size( my_array ) );
    BOOST_CHECK_EQUAL( empty( my_array ), false );

    BOOST_CHECK_EQUAL( begin( ca ), ca );
    BOOST_CHECK_EQUAL( end( ca ), ca + size( ca ) );
    BOOST_CHECK_EQUAL( empty( ca ),false );

    const char A[] = "\0A";
    BOOST_CHECK_EQUAL( boost::size(A), 3 );
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add( BOOST_TEST_CASE( &check_array ) );

    return test;
}





