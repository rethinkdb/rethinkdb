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


#include <boost/assign/list_inserter.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/functional.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <utility>
#include <stdexcept>
#include <cstdlib>


namespace ba = boost::assign;

void function_ptr( int )
{
    // do nothing
}

struct functor
{
    template< class T >
    void operator()( T ) const
    { 
        // do nothing
    }
};



void check_list_inserter()
{
    using namespace std;
    using namespace boost;
    using namespace boost::assign;
    vector<int> v;
    
    //
    // @note: cast only necessary on CodeWarrior
    //
    make_list_inserter( (void (*)(int))&function_ptr )( 5 ),3;
    make_list_inserter( functor() )( 4 ),2;
    
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
// BCB would crash
    typedef void (vector<int>::* push_back_t)(const int&);
    push_back_t push_back_func = &vector<int>::push_back;
    make_list_inserter( bind( push_back_func, &v, _1 ) )( 6 ),4;
#else
    make_list_inserter( bind( &vector<int>::push_back, &v, _1 ) )( 6 ),4;
#endif 

    BOOST_CHECK_EQUAL( v.size(), 2u );
    BOOST_CHECK_EQUAL( v[0], 6 );
    BOOST_CHECK_EQUAL( v[1], 4 );
    
    push_back( v ) = 1,2,3,4,5;
    BOOST_CHECK_EQUAL( v.size(), 7u );
    BOOST_CHECK_EQUAL( v[6], 5 );
    
    push_back( v )(6).repeat( 10, 7 )(8);
    BOOST_CHECK_EQUAL( v.size(), 19u );
    BOOST_CHECK_EQUAL( v[18], 8 );
    BOOST_CHECK_EQUAL( v[8], 7 ); 
    BOOST_CHECK_EQUAL( v[16], 7 ); 

#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1200)
    push_back( v ) = repeat_fun( 10, &rand );
#else
    push_back( v ).repeat_fun( 10, &rand );
#endif

    BOOST_CHECK_EQUAL( v.size(), 29u );

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1200) || BOOST_WORKAROUND(__SUNPRO_CC, <= 0x580 )
    push_back( v )(1).repeat( 10, 2 )(3);
#else
    push_back( v ) = 1,repeat( 10, 2 ),3;
#endif
    BOOST_CHECK_EQUAL( v.size(), 41u );

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1200) || BOOST_WORKAROUND(__SUNPRO_CC, <= 0x580 )
    push_back( v )(1).repeat_fun( 10, &rand )(2);  
#else
    push_back( v ) = 1,repeat_fun( 10, &rand ),2;
#endif

    BOOST_CHECK_EQUAL( v.size(), 53u );
    
    typedef map<string,int> map_t;
    typedef map_t::value_type V;
    map_t m;

    make_list_inserter( assign_detail::call_insert< map_t >( m ) )
                      ( V("bar",3) )( V("foo", 2) );
    BOOST_CHECK_EQUAL( m.size(), 2u );
    BOOST_CHECK_EQUAL( m["foo"], 2 );

    
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564)) || BOOST_WORKAROUND(BOOST_MSVC, <=1300)
#else

    typedef vector<int>                   score_type;
    typedef map<string,score_type>        team_score_map;
    typedef std::pair<string,score_type>  score_pair;
    team_score_map team_score;
    insert( team_score )( "Team Foo",    list_of(1)(1)(0) )
                        ( "Team Bar",    list_of(0)(0)(0) )
                        ( "Team FooBar", list_of(0)(0)(1) ); 
    BOOST_CHECK_EQUAL( team_score.size(), 3u );
    BOOST_CHECK_EQUAL( team_score[ "Team Foo" ][1], 1 );
    BOOST_CHECK_EQUAL( team_score[ "Team Bar" ][0], 0 );
   
    team_score = list_of< score_pair >
                        ( "Team Foo",    list_of(1)(1)(0) )
                        ( "Team Bar",    list_of(0)(0)(0) )
                        ( "Team FooBar", list_of(0)(0)(1) );
    BOOST_CHECK_EQUAL( team_score.size(), 3u );
    BOOST_CHECK_EQUAL( team_score[ "Team Foo" ][1], 1 );
    BOOST_CHECK_EQUAL( team_score[ "Team Bar" ][0], 0 );

#endif
                        
}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_list_inserter ) );

    return test;
}

