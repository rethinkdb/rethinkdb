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

#define PTR_LIST_TEST 1
#define PTR_CONTAINER_DEBUG 0
        
#include <boost/test/unit_test.hpp>
#include "sequence_test_data.hpp"
#include <boost/ptr_container/ptr_list.hpp>

void test_list()
{
    
    reversible_container_test< ptr_list<Base>, Base, Derived_class >();
    reversible_container_test< ptr_list<Value>, Value, Value >();
    reversible_container_test< ptr_list< nullable<Base> >, Base, Derived_class >();
    reversible_container_test< ptr_list< nullable<Value> >, Value, Value >();

    container_assignment_test< ptr_list<Base>, ptr_list<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_list< nullable<Base> >, 
                               ptr_list< nullable<Derived_class> >, 
                               Derived_class>();
    container_assignment_test< ptr_list< nullable<Base> >, 
                               ptr_list<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_list<Base>, 
                               ptr_list< nullable<Derived_class> >, 
                               Derived_class>();                           

    test_transfer< ptr_list<Derived_class>, ptr_list<Base>, Derived_class>();
      
    random_access_algorithms_test< ptr_list<int> >();
    ptr_list<int> list;
    list.push_back( new int(0) );
    list.push_back( new int(2) );
    list.push_back( new int(1) );
    list.push_front( new int(3) );
    list.push_front( std::auto_ptr<int>( new int(42) ) );
    list.reverse();

}



using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_list ) );

    return test;
}




