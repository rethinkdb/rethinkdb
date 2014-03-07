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
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <queue>
#include <boost/array.hpp>

void check_list_of()
{
    using namespace std;
    using namespace boost;
    using namespace boost::assign;
    
    vector<int>         v = list_of(1)(2)(3)(4).to_container( v );
    set<int>            s = list_of(1)(2)(3)(4).to_container( s );  
    map<int,int>        m = map_list_of(1,2)(2,3).to_container( m );
    stack<int>         st = list_of(1)(2)(3)(4).to_adapter( st );
    queue<int>         q  = list_of(1)(2)(3)(4).to_adapter( q ); 
    array<int,4>       a  = list_of(1)(2)(3)(4).to_array( a );
    const vector<int>  v2 = list_of(1).to_container( v2 );
    const array<int,1> a2 = list_of(1).to_array( a2 );
}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_list_of ) );

    return test;
}


