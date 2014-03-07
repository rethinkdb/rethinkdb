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
#include <boost/ptr_container/ptr_unordered_set.hpp>

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

template< class Cont, class T >
void test_unordered_interface()
{
    Cont c;
    T* t = new T;
    c.insert( t );
    typename Cont::local_iterator i = c.begin( 0 );
    typename Cont::const_local_iterator ci = i;
    ci = c.cbegin( 0 );
    i = c.end( 0 );
    ci = c.cend( 0 );
    typename Cont::size_type s = c.bucket_count();
    s = c.max_bucket_count();
    s = c.bucket_size( 0 );
    s = c.bucket( *t );
    float f = c.load_factor();
    f       = c.max_load_factor();
    c.max_load_factor(f);
    c.rehash(1000);
} 



template< class PtrSet > 
void test_erase()
{
    PtrSet s;
    typedef typename PtrSet::key_type T;

    T t;
    s.insert ( new T );
    T* t2 = t.clone();
    s.insert ( t2 );
    s.insert ( new T );
    BOOST_CHECK_EQUAL( s.size(), 3u ); 
    BOOST_CHECK_EQUAL( hash_value(t), hash_value(*t2) );
    BOOST_CHECK_EQUAL( t, *t2 );

    typename PtrSet::iterator i = s.find( t );

    BOOST_CHECK( i != s.end() );
    unsigned n = s.erase( t );
    BOOST_CHECK( n > 0 );   
}



void test_set()
{    
    srand( 0 );
    ptr_set_test< ptr_unordered_set<Base>, Base, Derived_class, false >();
    ptr_set_test< ptr_unordered_set<Value>, Value, Value, false >();

    ptr_set_test< ptr_unordered_multiset<Base>, Base, Derived_class, false >();
    ptr_set_test< ptr_unordered_multiset<Value>, Value, Value, false >();

    test_copy< ptr_unordered_set<Base>, ptr_unordered_set<Derived_class>, 
               Derived_class>();
    test_copy< ptr_unordered_multiset<Base>, ptr_unordered_multiset<Derived_class>, 
               Derived_class>();

    test_transfer< ptr_unordered_set<Derived_class>, ptr_unordered_set<Base>, Derived_class>();
    test_transfer< ptr_unordered_multiset<Derived_class>, ptr_unordered_multiset<Base>, Derived_class>();
    
    ptr_unordered_set<int> set;

    BOOST_CHECK_THROW( set.insert( 0 ), bad_ptr_container_operation );
    set.insert( new int(0) );
    std::auto_ptr<int> ap( new int(1) );
    set.insert( ap );
    BOOST_CHECK_THROW( (set.replace(set.begin(), 0 )), bad_ptr_container_operation );
    BOOST_CHECK_THROW( (set.replace(set.begin(), std::auto_ptr<int>(0) )), bad_ptr_container_operation );

    test_unordered_interface< ptr_unordered_set<Base>, Derived_class >();
    test_unordered_interface< ptr_unordered_multiset<Base>, Derived_class >();

    test_erase< ptr_unordered_set<Base> >();
    test_erase< ptr_unordered_multiset<Base> >();
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_set ) );

    return test;
}






