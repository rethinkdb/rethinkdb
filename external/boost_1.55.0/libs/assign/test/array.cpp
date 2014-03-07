// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/assign/
//

#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND( __BORLANDC__, BOOST_TESTED_AT(0x564) )
#  pragma warn -8091 // supress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/assign/list_of.hpp>
#include <boost/array.hpp>
#include <boost/test/test_tools.hpp>
#include <iostream>
#include <algorithm>
#include <iterator>



void check_array()
{
    using namespace std;
    using namespace boost;
    using namespace boost::assign;

    typedef array<float,6> Array;


#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))   
    Array a = list_of(1)(2)(3)(4)(5)(6).to_array(a);
#else
    Array a = list_of(1)(2)(3)(4)(5)(6);
#endif

    BOOST_CHECK_EQUAL( a[0], 1 );
    BOOST_CHECK_EQUAL( a[5], 6 );
    // last element is implicitly 0
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
    Array a2 = list_of(1)(2)(3)(4)(5).to_array(a2);
#else
    Array a2 = list_of(1)(2)(3)(4)(5);
#endif 
    BOOST_CHECK_EQUAL( a2[5], 0 );
    // two last elements are implicitly
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))   
    a2 = list_of(1))(2)(3)(4).to_array(a2);
#else    
    a2 = list_of(1)(2)(3)(4);
#endif    
    BOOST_CHECK_EQUAL( a2[4], 0 );
    BOOST_CHECK_EQUAL( a2[5], 0 );
    // too many arguments
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))

    BOOST_CHECK_THROW( a2 = list_of(1)(2)(3)(4)(5)(6)(6).to_array(a2),
                       assignment_exception );
#else
    BOOST_CHECK_THROW( a2 = list_of(1)(2)(3)(4)(5)(6)(7), assignment_exception );
#endif
    
        
}


#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;


test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_array ) );

    return test;
}

