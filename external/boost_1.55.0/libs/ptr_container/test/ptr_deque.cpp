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
#include "sequence_test_data.hpp"
#include <boost/ptr_container/ptr_deque.hpp>

void test_ptr_deque()
{
    reversible_container_test< ptr_deque<Base>, Base, Derived_class >();
    reversible_container_test< ptr_deque<Value>, Value, Value >();
    reversible_container_test< ptr_deque< nullable<Base> >, Base, Derived_class >();
    reversible_container_test< ptr_deque< nullable<Value> >, Value, Value >();

    container_assignment_test< ptr_deque<Base>, ptr_deque<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_deque< nullable<Base> >, 
                               ptr_deque< nullable<Derived_class> >, 
                               Derived_class>();
    container_assignment_test< ptr_deque< nullable<Base> >, 
                               ptr_deque<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_deque<Base>, 
                               ptr_deque< nullable<Derived_class> >, 
                               Derived_class>();                           

    test_transfer< ptr_deque<Derived_class>, ptr_deque<Base>, Derived_class>();
    
    random_access_algorithms_test< ptr_deque<int> >();
    ptr_deque<int> di;
    di.push_front( new int(0) );
    BOOST_CHECK_EQUAL( di.size(), 1u );
    di.push_front( std::auto_ptr<int>( new int(1) ) );
    BOOST_CHECK_EQUAL( di.size(), 2u ); 
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_ptr_deque ) );

    return test;
}

