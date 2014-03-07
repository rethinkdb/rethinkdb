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
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/assign/list_inserter.hpp>

void test_ptr_vector()
{ 
    reversible_container_test< ptr_vector<Base>, Base, Derived_class >();
    reversible_container_test< ptr_vector<Value>, Value, Value >();

#ifdef BOOST_NO_SFINAE
#else
    reversible_container_test< ptr_vector< nullable<Base> >, Base, Derived_class >();
    reversible_container_test< ptr_vector< nullable<Value> >, Value, Value >();
#endif    

    container_assignment_test< ptr_vector<Base>, ptr_vector<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_vector< nullable<Base> >, 
                               ptr_vector< nullable<Derived_class> >, 
                               Derived_class>();
    container_assignment_test< ptr_vector< nullable<Base> >, 
                               ptr_vector<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_vector<Base>, 
                               ptr_vector< nullable<Derived_class> >, 
                               Derived_class>();                           
                               
    test_transfer< ptr_vector<Derived_class>, ptr_vector<Base>, Derived_class>();
    test_transfer< ptr_vector<Derived_class>, ptr_list<Base>, Derived_class>();
     
    random_access_algorithms_test< ptr_vector<int> >();


    ptr_vector<int> vec( 100u );
    BOOST_CHECK( vec.capacity() >= 100u );

#ifdef BOOST_PTR_CONTAINER_NO_EXCEPTIONS    
#else

    BOOST_CHECK_THROW( vec.push_back(0), bad_ptr_container_operation );
    BOOST_CHECK_THROW( (vec.insert( vec.begin(), 0 )), bad_ptr_container_operation );
    BOOST_CHECK_THROW( vec.at( 42 ), bad_ptr_container_operation );
    vec.push_back( new int(0) );
    BOOST_CHECK_THROW( (vec.replace(10u, new int(0))), bad_ptr_container_operation );
    BOOST_CHECK_THROW( (vec.replace(0u, 0)), bad_ptr_container_operation ); 
    BOOST_CHECK_THROW( (vec.replace(vec.begin(), 0 )), bad_ptr_container_operation );

#endif

    vec.clear();
    assign::push_back( vec )( new int(2) )
                            ( new int(4) )
                            ( new int(6) )
                            ( new int(8) );
    ptr_vector<int> vec2;
    assign::push_back( vec2 )
                        ( new int(1) )
                        ( new int(3) )
                        ( new int(5) )
                        ( new int(7) );
    BOOST_CHECK_EQUAL( vec.size(), vec2.size() );
    BOOST_CHECK( vec > vec2 );
    BOOST_CHECK( vec != vec2 );
    BOOST_CHECK( !(vec == vec2) );
    BOOST_CHECK( vec2 < vec );
    BOOST_CHECK( vec2 <= vec );
    BOOST_CHECK( vec >= vec2 );

    const int data_size = 10;
    int** array = new int*[data_size];
    for( int i = 0; i != data_size; ++i ) 
        array[i] = new int(i);
        
    vec.transfer( vec.begin(), array, data_size );
    int** array2 = vec.c_array();
    BOOST_CHECK( array2 != array );
    
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_ptr_vector ) );

    return test;
}




