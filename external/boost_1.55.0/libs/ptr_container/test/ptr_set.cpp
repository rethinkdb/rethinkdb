//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//
 
#include <boost/test/unit_test.hpp>
#include "associative_test_data.hpp"
#include <boost/ptr_container/ptr_set.hpp>

template< class SetDerived, class SetBase, class T >
void test_transfer()
{
    SetBase to;
    SetDerived from;
    from.insert( new T );
    from.insert( new T );
    transfer_test( from, to );
}

template< class BaseContainer, class DerivedContainer, class Derived >
void test_copy()
{
    DerivedContainer derived;
    derived.insert( new Derived );
    derived.insert( new Derived );

    BaseContainer base( derived );
    BOOST_CHECK_EQUAL( derived.size(), base.size() );
    base.clear();
    base = derived;
    BOOST_CHECK_EQUAL( derived.size(), base.size() );
    base = base;
}



template< class PtrSet > 
void test_erase()
{
    PtrSet s;
    typedef typename PtrSet::key_type T;

    T t;
    T* t2 = t.clone();
    s.insert ( new T );
    s.insert ( t2 );
    s.insert ( new T );
    BOOST_CHECK_EQUAL( s.size(), 3u ); 
    BOOST_CHECK_EQUAL( t, *t2 );
    BOOST_CHECK( ! (t < *t2) );
    BOOST_CHECK( ! (*t2 < t) );
    BOOST_CHECK_EQUAL( t, *t2 );
    
    unsigned n = s.erase( t );
    BOOST_CHECK( n > 0 );   
}



void test_set()
{    
    srand( 0 );
    ptr_set_test< ptr_set<Base>, Base, Derived_class, true >();
    ptr_set_test< ptr_set<Value>, Value, Value, true >();

    ptr_set_test< ptr_multiset<Base>, Base, Derived_class, true >();
    ptr_set_test< ptr_multiset<Value>, Value, Value, true >();

    test_copy< ptr_set<Base>, ptr_set<Derived_class>, 
               Derived_class>();
    test_copy< ptr_multiset<Base>, ptr_multiset<Derived_class>, 
               Derived_class>();
    
    test_transfer< ptr_set<Derived_class>, ptr_set<Base>, Derived_class>();
    test_transfer< ptr_multiset<Derived_class>, ptr_multiset<Base>, Derived_class>();
    
    ptr_set<int> set;

    BOOST_CHECK_THROW( set.insert( 0 ), bad_ptr_container_operation );
    set.insert( new int(0) );
    std::auto_ptr<int> ap( new int(1) );
    set.insert( ap );
    BOOST_CHECK_THROW( (set.replace(set.begin(), 0 )), bad_ptr_container_operation );
    BOOST_CHECK_THROW( (set.replace(set.begin(), std::auto_ptr<int>(0) )), bad_ptr_container_operation );

    test_erase< ptr_set<Base> >();
    test_erase< ptr_multiset<Base> >();
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_set ) );

    return test;
}






