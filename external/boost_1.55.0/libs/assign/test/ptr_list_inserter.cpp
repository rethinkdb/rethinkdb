// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2006. Use, modification and
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


#include <boost/assign/ptr_list_inserter.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <typeinfo>

struct Foo
{
    int i;
    
    Foo() : i(0)
    { }
    Foo( int i ) : i(i)
    { }
    Foo( int i, int ) : i(i)
    { }
    Foo( const char*, int i, int ) : i(i)
    { }

    virtual ~Foo()
    { }
};

struct FooBar : Foo
{
    FooBar( int i ) : Foo(i)
    { }
    
    FooBar( int i, const char* )
    { }
};

inline bool operator<( const Foo& l, const Foo& r )
{
    return l.i < r.i;
}

void check_ptr_list_inserter()
{
    using namespace std;
    using namespace boost;
    using namespace boost::assign;

    ptr_deque<Foo> deq;
    ptr_push_back( deq )()();
    BOOST_CHECK( deq.size() == 2u );
    
    ptr_push_front( deq )( 3 )( 42, 42 )( "foo", 42, 42 );
    BOOST_CHECK( deq.size() == 5u );

    ptr_set<Foo> a_set;
    ptr_insert( a_set )()( 1 )( 2, 2 )( "foo", 3, 3 );
    BOOST_CHECK( a_set.size() == 4u );
    ptr_insert( a_set )()()()();
    BOOST_CHECK( a_set.size() == 4u ); // duplicates not inserted

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

    ptr_push_back<FooBar>( deq )( 42, "42" );
    BOOST_CHECK_EQUAL( deq.size(), 6u );
    BOOST_CHECK( typeid(deq[5u]) == typeid(FooBar) );

    ptr_push_front<FooBar>( deq )( 42, "42" );
    BOOST_CHECK_EQUAL( deq.size(), 7u );
    BOOST_CHECK( typeid(deq[0]) == typeid(FooBar) );

    ptr_insert<FooBar>( a_set )( 4 );
    BOOST_CHECK( a_set.size() == 5u );
    BOOST_CHECK( typeid(*--a_set.end()) == typeid(FooBar) );

#endif

}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_ptr_list_inserter ) );

    return test;
}

