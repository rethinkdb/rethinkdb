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

#include <boost/assign/std.hpp>
#include <boost/test/test_tools.hpp>
#include <utility>
#include <string>

using namespace std;
using namespace boost::assign;  
    
template< typename K, typename V >
inline pair<K,V> P( K k, V v )
{
    return make_pair( k, v );
}

struct three
{
    three( int, int, int ) { }
    three( const string&, const string&, const string& ) { }
};

struct four
{
    four( int, int, int, int ) { }
    four( const string&, const string&, const string&, const string& ) { }
};

struct five
{
    five( int, int, int, int, int ) { }
    five( const string&, const string&, const string&,  
          const string&, const string& ) { }
};



template< class C >
void test_int_sequence()
{
    C c;
    
    BOOST_CHECK_EQUAL( c.size(), 0u );
    c +=1,2,3,4,5,6,7,8,9,10;
    BOOST_CHECK_EQUAL( c.size(), 10u );
}



template< class C >
void test_string_sequence()
{
    C c;

    BOOST_CHECK_EQUAL( c.size(), 0u );
    c += "1","2","3","4","5","6","7","8","9","10";
    BOOST_CHECK_EQUAL( c.size(), 10u );
}



typedef pair<string,int> tuple; 

template< class C >
void test_tuple_sequence()
{
    C c;
    
    BOOST_CHECK_EQUAL( c.size(), 0u );
    c += P("1",1), P("2",2), P("3",3), P("4",4), P("5",5), P("6",6), 
         P("7",7), P("8",8), P("9",9), P("10",10);
    BOOST_CHECK_EQUAL( c.size(), 10u );
}



template< class M >
void test_map()
{
    M m;
    m += P( "january",   31 ), P( "february", 28 ),
         P( "march",     31 ), P( "april",    30 ),
         P( "may",       31 ), P( "june",     30 ),
         P( "july",      31 ), P( "august",   31 ),
         P( "september", 30 ), P( "october",  31 ),
         P( "november",  30 ), P( "december", 31 );
    BOOST_CHECK_EQUAL( m.size(), 12u );
    m.clear();
    insert( m )
        ( "january",   31 )( "february", 28 )
        ( "march",     31 )( "april",    30 )
        ( "may",       31 )( "june",     30 )
        ( "july",      31 )( "august",   31 )
        ( "september", 30 )( "october",  31 )
        ( "november",  30 )( "december", 31 );
    BOOST_CHECK_EQUAL( m.size(), 12u );
}



void test_tuple()
{
    vector<three>    v_three;
    vector<four>     v_four;
    vector<five>     v_five;

    push_back( v_three ) (1,2,3) ("1","2","3");
    push_back( v_four ) (1,2,3,4) ("1","2","3","4");
    push_back( v_five ) (1,2,3,4,5) ("1","2","3","4","5");
    BOOST_CHECK_EQUAL( v_three.size(), 2u );
    BOOST_CHECK_EQUAL( v_four.size(), 2u );
    BOOST_CHECK_EQUAL( v_five.size(), 2u );

}



void check_std()
{
    test_int_sequence< deque<int> >();
    test_int_sequence< list<int> >();          
    test_int_sequence< vector<int> >();       
    test_int_sequence< set<int> >();          
    test_int_sequence< multiset<int> >();     
    test_int_sequence< stack<int> >();        
    test_int_sequence< queue<int> >();        
    test_int_sequence< priority_queue<int> >();
            
    test_string_sequence< deque<string> >();             
    test_string_sequence< list<string> >();              
    test_string_sequence< vector<string> >();            
    test_string_sequence< set<string> >();               
    test_string_sequence< multiset<string> >();          
    test_string_sequence< stack<string> >();             
    test_string_sequence< queue<string> >();             
    test_string_sequence< priority_queue<string> >();    

    test_tuple_sequence< deque<tuple> >();             
    test_tuple_sequence< list<tuple> >();              
    test_tuple_sequence< vector<tuple> >();            
    test_tuple_sequence< set<tuple> >();               
    test_tuple_sequence< multiset<tuple> >();          
    test_tuple_sequence< stack<tuple> >();             
    test_tuple_sequence< queue<tuple> >();             
    test_tuple_sequence< priority_queue<tuple> >();    
    test_tuple();
    
    deque<int>          di; 
    push_back( di )( 1 );
    push_front( di )( 2 );
    BOOST_CHECK_EQUAL( di[0], 2 );
    BOOST_CHECK_EQUAL( di[1], 1 );
    
    list<int>           li; 
    push_back( li )( 2 );
    push_front( li )( 1 );
    BOOST_CHECK_EQUAL( li.front(), 1 );
    BOOST_CHECK_EQUAL( li.back(), 2 );
    
    vector<int>         vi; 
    push_back( vi ) = 2,3;
    BOOST_CHECK_EQUAL( vi[0], 2 );
    BOOST_CHECK_EQUAL( vi[1], 3 );
    
    set<int>            si; 
    insert( si )( 4 );
    BOOST_CHECK_EQUAL( *si.find( 4 ), 4 );
    
    multiset<int>       msi; 
    insert( msi )( 5 );
    BOOST_CHECK_EQUAL( *msi.find( 5 ), 5 );
    
    stack<int>          sti; 
    push( sti )( 6 );
    BOOST_CHECK_EQUAL( sti.top(), 6 );
    
    queue<int>          qi; 
    push( qi )( 7 );
    BOOST_CHECK_EQUAL( qi.back(), 7 );
    
    priority_queue<int> pqi; 
    push( pqi )( 8 );
    BOOST_CHECK_EQUAL( pqi.top(), 8 );
    
    test_map< map<string,int> >();
    test_map< multimap<string,int> >();

}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_std ) );

    return test;
}

