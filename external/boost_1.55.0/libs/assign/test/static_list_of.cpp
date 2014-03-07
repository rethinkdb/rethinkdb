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
#include <boost/array.hpp>
#include <boost/test/test_tools.hpp>
#include <algorithm>
#include <iostream>

template< class Range >
void print( const Range& r )
{
    std::cout << "\n printing " << typeid(r).name() << " \n"; 
    std::cout << "\n";
    for( typename Range::iterator i = r.begin(), e = r.end();
         i !=e; ++i )
         std::cout << " " << *i;
}

template< class Range >
void sort( const Range& r )
{
    std::cout << "\n sorting " << typeid(r).name() << " \n"; 
    std::sort( r.begin(), r.end() );
    print( r );
}

template< class Range, class Pred >
void sort( const Range& r, Pred pred )
{
    std::cout << "\n sorting " << typeid(r).name() << " \n"; 
    std::sort( r.begin(), r.end(), pred );
    print( r );
}

template< class Range >
typename Range::const_iterator max_element( const Range& r )
{
    return std::max_element( r.begin(), r.end() );
}



void check_static_list_of()
{
    using namespace boost::assign;
    
    BOOST_CHECK( cref_list_of<5>( 1 )( 2 )( 3 )( 4 ).size() == 4 );
    
    int a=1,b=5,c=3,d=4,e=2,f=9,g=0,h=7;

    int& max = *max_element( ref_list_of<8>(a)(b)(c)(d)(e)(f)(g)(h) );
    BOOST_CHECK_EQUAL( max, f );
    max = 8;
    BOOST_CHECK_EQUAL( f, 8 );
    const int& const_max = *max_element( cref_list_of<8>(a)(b)(c)(d)(e)(f)(g)(h) );
    BOOST_CHECK_EQUAL( max, const_max );

    print( ref_list_of<8>(a)(b)(c)(d)(e)(f)(g)(h) );
    print( cref_list_of<8>(a)(b)(c)(d)(e)(f)(g)(h) );

    boost::array<int,4> array = cref_list_of<4>(1)(2)(3)(4);

    BOOST_CHECK_EQUAL( array[0], 1 );
    BOOST_CHECK_EQUAL( array[3], 4 );
    //
    //print( cref_list_of<5>( "foo" )( "bar" )( "foobar" ) );
    //
}

#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_static_list_of ) );

    return test;
}


