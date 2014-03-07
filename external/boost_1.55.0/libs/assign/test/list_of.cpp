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
#include <boost/array.hpp>
#include <algorithm>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <stack>
#include <string>
#include <cstdlib>
#include <complex>

struct nothing
{
    template< class T >
    void operator()( T )
    { }
    
};

template< class Range >
void for_each( const Range& r )
{
    std::for_each( r.begin(), r.end(), nothing() );
}

namespace ba = boost::assign;
    
template< class C >
void test_sequence_list_of_string()
{
#if BOOST_WORKAROUND(BOOST_MSVC, <=1300)
    const C c = ba::list_of( "foo" )( "bar" ).to_container( c );   
#else
    const C c = ba::list_of( "foo" )( "bar" );   
#endif
    BOOST_CHECK_EQUAL( c.size(), 2u );
}

struct parameter_list
{
    int val;
    
    template< class T >
    parameter_list( T )
    : val(0)
    { }
    
    template< class T >
    parameter_list( const T&, int )
    : val(1)
    { }
};

template< class C >
void test_sequence_list_of_int()
{
    using namespace std;
#if BOOST_WORKAROUND(BOOST_MSVC, <=1300)

    const C c = ba::list_of<int>(1)(2)(3)(4).to_container( c );
    const C c2 = ba::list_of(1)(2)(3)(4).to_container( c2 );
    BOOST_CHECK_EQUAL( c.size(), 4u );
    BOOST_CHECK_EQUAL( c2.size(), 4u );
    C c3 = ba::list_of(1).repeat( 1, 2 )(3).to_container( c3 );
    BOOST_CHECK_EQUAL( c3.size(), 3u );
        
    c3 = ba::list_of(1).repeat_fun( 10, &rand )(2)(3).to_container( c3 );
    BOOST_CHECK_EQUAL( c3.size(), 13u );

#else

    const C c = ba::list_of<int>(1)(2)(3)(4);
    const C c2 = ba::list_of(1)(2)(3)(4);
    BOOST_CHECK_EQUAL( c.size(), 4u );
    BOOST_CHECK_EQUAL( c2.size(), 4u );
    C c3 = ba::list_of(1).repeat( 1, 2 )(3);
    BOOST_CHECK_EQUAL( c3.size(), 3u );
        
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
    // BCB fails to use operator=() directly, 
    // it must be worked around using e.g. auxiliary variable
    C aux = ba::list_of(1).repeat_fun( 10, &rand )(2)(3);
    BOOST_CHECK_EQUAL( aux.size(), 13u );
    c3 = aux;
    BOOST_CHECK_EQUAL( c3.size(), 13u );
#else
    c3 = ba::list_of(1).repeat_fun( 10, &rand )(2)(3);
    BOOST_CHECK_EQUAL( c3.size(), 13u );
#endif

#endif

    parameter_list p( ba::list_of(1)(2), 3u );
    BOOST_CHECK_EQUAL( p.val, 1 );

}

template< class C >
void test_map_list_of()
{
    const C c  = ba::list_of< std::pair<std::string,int> >( "foo", 1 )( "bar", 2 )( "buh", 3 )( "bah", 4 );
    BOOST_CHECK_EQUAL( c.size(), 4u );
    const C c2  = ba::map_list_of( "foo", 1 )( "bar", 2 )( "buh", 3 )( "bah", 4 );
    BOOST_CHECK_EQUAL( c2.size(), 4u );
}

void test_vector_matrix()
{
    using namespace boost;
    using namespace boost::assign;
    using namespace std;

#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)  || BOOST_WORKAROUND(BOOST_MSVC, <=1300)
#else    
      
    const int              sz = 3;
    typedef array<int,sz>   row3;
    typedef array<row3,sz>  matrix3x3;
    

    matrix3x3 m = list_of( list_of(1)(2)(3) )
                         ( list_of(4)(5)(6) )
                         ( list_of(7)(8)(9) );

    for( int i = 0; i != sz; ++i )
        for( int j = 0; j != sz; ++j )
            BOOST_CHECK_EQUAL( m[i][j], i*sz + j + 1 );

    typedef vector<int>  row;
    typedef vector<row>  matrix;
    
    //
    // note: some libraries need a little help
    //       with the conversion, hence the 'row' template parameter. 
    //
    matrix m2 = list_of< row >( list_of(1)(2)(3) )
                              ( list_of(4)(5) )
                              ( list_of(6) );
    
    for( int i = 0; i != sz; ++i )
        for( int j = 0; j != sz - i; ++j )
            BOOST_CHECK_EQUAL( m[i][j], i*sz + j + 1 );

#endif  
  
}

void test_map_list_of()
{
/*
    maybe in the future...
   
    using namespace std;
    using namespace boost::assign;
     
    typedef vector<int>                   score_type;
    typedef map<string,score_type>        team_score_map;
 
    team_score_map team_score = map_list_of
                        ( "Team Foo",    list_of(1)(1)(0) )
                        ( "Team Bar",    list_of(0)(0)(0) )
                        ( "Team FooBar", list_of(0)(0)(1) );
    BOOST_CHECK_EQUAL( team_score.size(), 3 );
    BOOST_CHECK_EQUAL( team_score[ "Team Foo" ][1], 1 );
    BOOST_CHECK_EQUAL( team_score[ "Team Bar" ][0], 0 );
*/

}

/*
void test_complex_list_of()
{
    typedef std::complex<float> complex_t;
    std::vector<complex_t> v;
    v = ba::list_of<complex_t>(1,2)(2,3)(4,5)(0).
          repeat_from_to( complex_t(0,0), complex_t(10,10), complex_t(1,1) ); 
}
*/

struct five
{
    five( int, int, int, int, int )
    {
    }
};

void test_list_of()
{
    ba::list_of< five >(1,2,3,4,5)(6,7,8,9,10);
    
/* Maybe this could be usefull in a later version?

    // an anonymous lists, fulfills Range concept
    for_each( ba::list_of( T() )( T() )( T() ) );
    
    // non-anonymous lists
    ba::generic_list<T> list_1 = ba::list_of( T() );
    BOOST_CHECK_EQUAL( list_1.size(), 1 );
    ba::generic_list<T> list_2 = list_1 + ba::list_of( T() )( T() ) + list_1;
    BOOST_CHECK_EQUAL( list_2.size(), 4 );
    list_1 += list_2;
    BOOST_CHECK_EQUAL( list_1.size(), 5 );
*/
}

//
// @remark: ADL is required here, but it is a bit wierd to
//          open up namespace std. Perhaps Boost.Test needs a
//          better configuration option. 
//
namespace std
{
    template< class T, class Elem, class Traits >
    inline std::basic_ostream<Elem,Traits>& 
    operator<<( std::basic_ostream<Elem, Traits>& Os,
                const std::vector<T>& r )
    {
        return Os << ::boost::make_iterator_range( r.begin(),  r.end() );
    }
}

template <class Seq>
inline std::vector<int> as_seq( const Seq& s )
{
    std::vector<int> c;
    return s.to_container( c );
}

void test_comparison_operations()
{
    BOOST_CHECK_EQUAL( ba::list_of(0)(1)(2), as_seq(ba::list_of(0)(1)(2)) );
    BOOST_CHECK_NE( ba::list_of(0)(1)(2), as_seq(ba::list_of(-1)(1)(2)) );
    BOOST_CHECK_LT( ba::list_of(0)(1)(2), as_seq(ba::list_of(0)(1)(3)) );
    BOOST_CHECK_LE( ba::list_of(0)(1)(2), as_seq(ba::list_of(0)(1)(2)) );
    BOOST_CHECK_GT( ba::list_of(0)(1)(3), as_seq(ba::list_of(0)(1)(2)) );
    BOOST_CHECK_GE( ba::list_of(0)(1)(2), as_seq(ba::list_of(0)(1)(2)) );
    BOOST_CHECK_EQUAL( as_seq(ba::list_of(0)(1)(2)), ba::list_of(0)(1)(2) );
    BOOST_CHECK_NE( as_seq(ba::list_of(0)(1)(2)), ba::list_of(-1)(1)(2) );
    BOOST_CHECK_LT( as_seq(ba::list_of(0)(1)(2)), ba::list_of(0)(1)(3) );
    BOOST_CHECK_LE( as_seq(ba::list_of(0)(1)(2)), ba::list_of(0)(1)(2) );
    BOOST_CHECK_GT( as_seq(ba::list_of(0)(1)(3)), ba::list_of(0)(1)(2) );
    BOOST_CHECK_GE( as_seq(ba::list_of(0)(1)(2)), ba::list_of(0)(1)(2) );
}

void check_list_of()
{
    test_sequence_list_of_int< std::vector<int> >();
    test_sequence_list_of_int< std::list<int> >();
    test_sequence_list_of_int< std::deque<int> >();
    test_sequence_list_of_int< std::set<int> >();
    test_sequence_list_of_int< std::multiset<int> >();
    test_sequence_list_of_int< std::vector<float> >();

    test_sequence_list_of_string< std::vector<std::string> >();

    test_map_list_of< std::map<std::string,int> >();
    test_map_list_of< std::multimap<std::string,int> >();
    
    std::stack<std::string> s = ba::list_of( "Foo" )( "Bar" )( "FooBar" ).to_adapter( s );
    test_list_of();
    test_vector_matrix();
    test_comparison_operations();
}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_list_of ) );

    return test;
}


