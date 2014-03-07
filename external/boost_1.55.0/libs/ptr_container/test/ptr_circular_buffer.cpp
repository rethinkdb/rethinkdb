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
#include <boost/ptr_container/ptr_circular_buffer.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/assign/list_inserter.hpp>

template<class T>
struct set_capacity< ptr_circular_buffer<T> >
{
    void operator()( ptr_circular_buffer<T>& c ) const
    {
        c.set_capacity( 100u ); 
    }
};

void test_ptr_circular_buffer()
{ 
    reversible_container_test< ptr_circular_buffer<Base>, Base, Derived_class >();
    reversible_container_test< ptr_circular_buffer<Value>, Value, Value >();

#ifdef BOOST_NO_SFINAE
#else
    reversible_container_test< ptr_circular_buffer< nullable<Base> >, Base, Derived_class >();
    reversible_container_test< ptr_circular_buffer< nullable<Value> >, Value, Value >();
#endif    

    container_assignment_test< ptr_circular_buffer<Base>, ptr_circular_buffer<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_circular_buffer< nullable<Base> >, 
                               ptr_circular_buffer< nullable<Derived_class> >, 
                               Derived_class>();
    container_assignment_test< ptr_circular_buffer< nullable<Base> >, 
                               ptr_circular_buffer<Derived_class>, 
                               Derived_class>();
    container_assignment_test< ptr_circular_buffer<Base>, 
                               ptr_circular_buffer< nullable<Derived_class> >, 
                               Derived_class>();                           
                               
    test_transfer< ptr_circular_buffer<Derived_class>, ptr_circular_buffer<Base>, Derived_class>();
    test_transfer< ptr_circular_buffer<Derived_class>, ptr_list<Base>, Derived_class>();
     
    random_access_algorithms_test< ptr_circular_buffer<int> >();


    BOOST_MESSAGE( "starting ptr_circular_buffer test" );
    ptr_circular_buffer<int> vec( 100u );
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
    ptr_circular_buffer<int> vec2( 100u );
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

    BOOST_MESSAGE( "push_front test" );    
    assign::push_front( vec2 )
                      ( new int(2) )
                      ( new int(4) )
                      ( new int(6) )
                      ( new int(8) );
    BOOST_CHECK_EQUAL( vec2.size(), 8u );
    BOOST_CHECK_EQUAL( vec2[0], 8 );
    BOOST_CHECK_EQUAL( vec2.front(), 8 );
    BOOST_CHECK_EQUAL( vec2.back(), 7 );

    //vec2.linearize();
    vec2.rset_capacity( vec2.size() - 2u );
    vec2.rresize( 0 );
    //vec2.reverse();

    BOOST_MESSAGE( "when full test" );    

    ptr_circular_buffer<int> vec3;
    BOOST_CHECK_EQUAL( vec3.capacity(),  0u );
    vec3.set_capacity( 2u );
    BOOST_CHECK_EQUAL( vec3.capacity(),  2u );

    vec3.push_back( new int(1) );
    vec3.push_back( new int(2) );
    BOOST_CHECK_EQUAL( vec3.size(), 2u );
    BOOST_CHECK( vec3.full() ); 

    vec3.push_back( new int(3) );
    BOOST_CHECK_EQUAL( vec3.front(), 2 );
    BOOST_CHECK_EQUAL( vec3.back(), 3 );

    vec3.push_front( new int(4) );
    BOOST_CHECK_EQUAL( vec3.size(), 2u );
    BOOST_CHECK_EQUAL( vec3.front(), 4 );
    BOOST_CHECK_EQUAL( vec3.back(), 2 );
        
    vec3.insert( vec3.end(), new int(5) );
    BOOST_CHECK_EQUAL( vec3.front(), 2 );
    BOOST_CHECK_EQUAL( vec3.back(), 5 );

    vec3.rinsert( vec3.begin(), new int(6) );
    BOOST_CHECK_EQUAL( vec3.front(), 6 );
    BOOST_CHECK_EQUAL( vec3.back(), 2 );

    BOOST_MESSAGE( "transfer test" );    
    ptr_circular_buffer<int> vec4(2u);
    vec4.transfer( vec4.end(), vec3 );
    BOOST_CHECK_EQUAL( vec4.size(), 2u );
    BOOST_CHECK_EQUAL( vec3.size(), 0u );
    vec3.set_capacity(1u);
    vec3.transfer( vec3.end(), vec4 ); 
    BOOST_CHECK_EQUAL( vec4.size(), 0u );
    BOOST_CHECK_EQUAL( vec3.size(), 1u );
    BOOST_CHECK_EQUAL( vec3.front(), 2 );

    BOOST_MESSAGE( "rerase test" );   
    vec.rerase( vec.begin() );
    vec.rerase( boost::make_iterator_range( vec ) ); 

    BOOST_MESSAGE( "array test" );    
    const int data_size = 10;
    int** array = new int*[data_size];
    for( int i = 0; i != data_size; ++i ) 
        array[i] = new int(i);
        
    vec.transfer( vec.begin(), array, data_size );
    int** array2 = vec.c_array();
    BOOST_CHECK( array2 != array );

    ptr_circular_buffer<int>::array_range array_range = vec.array_one();
    array_range = vec.array_two();
    ptr_circular_buffer<int>::const_array_range const_array_range = array_range;
    const_array_range = const_cast< const ptr_circular_buffer<int>& >(vec).array_one();
    const_array_range = const_cast< const ptr_circular_buffer<int>& >(vec).array_two();
    
    BOOST_MESSAGE( "finishing ptr_circular_buffer test" );    
    
}



void test_circular_buffer()
{ 
    boost::circular_buffer<void*> b(25u);
    BOOST_CHECK_EQUAL( b.capacity(),  25u );
    b.push_back( 0 );
    BOOST_CHECK_EQUAL( b.size(),  1u );
    boost::circular_buffer<void*> b2( b.begin(), b.end() );
    BOOST_CHECK_EQUAL( b2.size(), b.size() );
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_circular_buffer ) );
    test->add( BOOST_TEST_CASE( &test_ptr_circular_buffer ) );

    return test;
}




