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

#include <boost/assign/list_of.hpp>
#include <boost/test/test_tools.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <map>

void check_tuple_list_of()
{
    using namespace boost::assign;

    typedef boost::tuple<int,std::string,int> tuple;

    std::vector<tuple> v = tuple_list_of( 1, "foo", 2 )( 3, "bar", 4 );
    BOOST_CHECK( v.size() == 2 );
    BOOST_CHECK( boost::get<0>( v[1] ) ==  3 );

    std::map<std::string,int> m = pair_list_of( "foo", 3 )( "bar", 5 );
    BOOST_CHECK( m.size() == 2 );
    BOOST_CHECK( m["foo"] == 3 ); 
}

#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_tuple_list_of ) );

    return test;
}


