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
 
#include "sequence_test_data.hpp"
#include <boost/ptr_container/ptr_container_adapter.hpp>
#include <list>


template< class T >
class my_list : public std::list<T>
{
    typedef BOOST_DEDUCED_TYPENAME std::list<T> base_class;
    
public:
/*
    my_list( const base_class::allocator_type& alloc = base_class::allocator_type() ) 
    : base_class( alloc ) {}               
    
    my_list( size_type n, const T& x, const base_class::allocator_type& alloc = base_class::allocator_type() ) 
    : base_class( n, x, alloc ) {}                                                   
    
    template< class InputIterator >                                                   
    my_list( InputIterator first, InputIterator last ) : base_class( first, last ) {}
*/
};

void test_container_adapter()
{
    typedef ptr_container_adapter< my_list<Base*> > base_ptr_list;
    typedef ptr_container_adapter< my_list<Value*> > value_ptr_list;
    
    typedef_test< base_ptr_list, Derived_class >();
    typedef_test< value_ptr_list, Value >();

//    reversible_container_test< base_ptr_list, Base, Derived_class >();
//    reversible_container_test< value_ptr_list, Value, Value >();
    
    base_ptr_list l;
    l.push_back( new Derived_class );
    l.push_back( new Derived_class );
    
//    algo_test< ptr_list<Value>, Value >();
//    algo_test_polymorphic< ptr_list<Base>, Derived_class >();
}

#include <boost/test/included/unit_test_framework.hpp> 

using boost::unit_test_framework::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_container_adapter ) );

    return test;
}



