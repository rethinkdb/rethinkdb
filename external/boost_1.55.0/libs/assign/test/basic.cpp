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

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // supress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/assign/std/vector.hpp>
#include <boost/assign/std/map.hpp>
#include <string>


using namespace std;
using namespace boost;
using namespace boost::assign;

void check_basic_usage()
{
    vector<int> v;
    v += 1,2,3,4,5,6,7,8,9;
    push_back( v )(10)(11);
    map<string,int> m;
    insert( m )( "foo", 1 )( "bar", 2 );
}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Assign Test Suite" );

    test->add( BOOST_TEST_CASE( &check_basic_usage ) );

    return test;
}

