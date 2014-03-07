//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>

#include <boost/test/test_tools.hpp>



namespace sc = boost::statechart;



struct A;
struct TypeInfoTest : sc::state_machine< TypeInfoTest, A > {};

struct B;
struct A : sc::simple_state< A, TypeInfoTest, B > {};

  struct B : sc::simple_state< B, A > {};


int test_main( int, char* [] )
{
  TypeInfoTest machine;
  machine.initiate();

  const TypeInfoTest::state_base_type & activeState =
    *machine.state_begin();
  const TypeInfoTest::state_base_type::id_type bType =
    activeState.dynamic_type();
  const TypeInfoTest::state_base_type::id_type aType =
    activeState.outer_state_ptr()->dynamic_type();

  BOOST_REQUIRE( bType == B::static_type() );
  BOOST_REQUIRE( bType != A::static_type() );
  BOOST_REQUIRE( aType == A::static_type() );
  BOOST_REQUIRE( aType != B::static_type() );

  #ifndef BOOST_STATECHART_USE_NATIVE_RTTI
  // Ensure that a null custom type id pointer can be of any type
  BOOST_REQUIRE( activeState.custom_dynamic_type_ptr< void >() == 0 );
  BOOST_REQUIRE( activeState.custom_dynamic_type_ptr< char >() == 0 );
  BOOST_REQUIRE( activeState.custom_dynamic_type_ptr< bool >() == 0 );
  BOOST_REQUIRE(
    activeState.outer_state_ptr()->custom_dynamic_type_ptr< void >() == 0 );
  BOOST_REQUIRE(
    activeState.outer_state_ptr()->custom_dynamic_type_ptr< char >() == 0 );
  BOOST_REQUIRE(
    activeState.outer_state_ptr()->custom_dynamic_type_ptr< bool >() == 0 );

  const char * bCustomType = "B";
  const char * aCustomType = "A";
  B::custom_static_type_ptr( bCustomType );
  A::custom_static_type_ptr( aCustomType );
  BOOST_REQUIRE( B::custom_static_type_ptr< char >() == bCustomType );
  BOOST_REQUIRE( A::custom_static_type_ptr< char >() == aCustomType );
  BOOST_REQUIRE(
    activeState.custom_dynamic_type_ptr< char >() == bCustomType );
  BOOST_REQUIRE(
    activeState.outer_state_ptr()->custom_dynamic_type_ptr< char >() ==
    aCustomType );

  // Ensure that a null custom type id pointer can be of any type
  bool * pNull = 0;
  B::custom_static_type_ptr( pNull );
  A::custom_static_type_ptr( pNull );
  BOOST_REQUIRE( activeState.custom_dynamic_type_ptr< char >() == 0 );
  BOOST_REQUIRE(
    activeState.outer_state_ptr()->custom_dynamic_type_ptr< char >() == 0 );
  #endif

  return 0;
}
