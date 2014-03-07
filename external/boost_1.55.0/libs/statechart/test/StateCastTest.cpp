//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/mpl/list.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/test/test_tools.hpp>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct EvToB : sc::event< EvToB > {};
struct EvToF : sc::event< EvToF > {};
struct EvCheck : sc::event< EvCheck > {};

struct A;
struct StateCastTest : sc::state_machine< StateCastTest, A >
{
  template< class State >
  void AssertInState()
  {
    BOOST_REQUIRE( state_downcast< const State * >() != 0 );
    BOOST_REQUIRE_NO_THROW( state_downcast< const State & >() );
    BOOST_REQUIRE( state_cast< const State * >() != 0 );
    BOOST_REQUIRE_NO_THROW( state_cast< const State & >() );
  }

  template< class State >
  void AssertNotInState()
  {
    BOOST_REQUIRE( state_downcast< const State * >() == 0 );
    BOOST_REQUIRE_THROW( state_downcast< const State & >(), std::bad_cast );
    BOOST_REQUIRE( state_cast< const State * >() == 0 );
    BOOST_REQUIRE_THROW( state_cast< const State & >(), std::bad_cast );
  }
};

template< class State, class FromState >
void AssertInState( const FromState & theState )
{
  BOOST_REQUIRE( theState.template state_downcast< const State * >() != 0 );
  BOOST_REQUIRE_NO_THROW( theState.template state_downcast< const State & >() );
  BOOST_REQUIRE( theState.template state_cast< const State * >() != 0 );
  BOOST_REQUIRE_NO_THROW( theState.template state_cast< const State & >() );
}

template< class State, class FromState >
void AssertNotInState( const FromState & theState )
{
  BOOST_REQUIRE( theState.template state_downcast< const State * >() == 0 );
  BOOST_REQUIRE_THROW( theState.template state_downcast< const State & >(), std::bad_cast );
  BOOST_REQUIRE( theState.template state_cast< const State * >() == 0 );
  BOOST_REQUIRE_THROW( theState.template state_cast< const State & >(), std::bad_cast );
}

struct B;
struct C;
struct D;
struct A : sc::simple_state< A, StateCastTest, mpl::list< C, D > >
{
  typedef sc::transition< EvToB, B > reactions;
};

  struct E;
  struct C : sc::simple_state< C, A::orthogonal< 0 >, E > {};

    struct E : sc::state< E, C >
    {
      typedef sc::custom_reaction< EvCheck > reactions;

      E( my_context ctx ) : my_base( ctx )
      {
        post_event( boost::intrusive_ptr< EvCheck >( new EvCheck() ) );
      }

      sc::result react( const EvCheck & );
    };

    struct F : sc::state< F, C >
    {
      typedef sc::custom_reaction< EvCheck > reactions;

      F( my_context ctx ) : my_base( ctx )
      {
        post_event( boost::intrusive_ptr< EvCheck >( new EvCheck() ) );
      }

      sc::result react( const EvCheck & );
    };

  struct G;
  struct D : sc::simple_state< D, A::orthogonal< 1 >, G > {};
  
    struct G : sc::simple_state< G, D > {};
    struct H : sc::simple_state< H, D > {};

struct B : sc::simple_state< B, StateCastTest >
{
  typedef sc::transition< EvToF, F > reactions;
};

sc::result E::react( const EvCheck & )
{
  AssertInState< A >( *this );
  AssertNotInState< B >( *this );
  AssertInState< C >( *this );
  AssertInState< D >( *this );
  AssertInState< E >( *this );
  AssertNotInState< F >( *this );
  AssertInState< G >( *this );
  AssertNotInState< H >( *this );
  return discard_event();
}

sc::result F::react( const EvCheck & )
{
  AssertInState< A >( *this );
  AssertNotInState< B >( *this );
  AssertInState< C >( *this );
  AssertInState< D >( *this );
  AssertNotInState< E >( *this );
  AssertInState< F >( *this );
  AssertInState< G >( *this );
  AssertNotInState< H >( *this );
  return discard_event();
}


int test_main( int, char* [] )
{
  StateCastTest machine;

  machine.AssertNotInState< A >();
  machine.AssertNotInState< B >();
  machine.AssertNotInState< C >();
  machine.AssertNotInState< D >();
  machine.AssertNotInState< E >();
  machine.AssertNotInState< F >();
  machine.AssertNotInState< G >();
  machine.AssertNotInState< H >();

  machine.initiate();

  machine.AssertInState< A >();
  machine.AssertNotInState< B >();
  machine.AssertInState< C >();
  machine.AssertInState< D >();
  machine.AssertInState< E >();
  machine.AssertNotInState< F >();
  machine.AssertInState< G >();
  machine.AssertNotInState< H >();

  machine.process_event( EvToB() );

  machine.AssertNotInState< A >();
  machine.AssertInState< B >();
  machine.AssertNotInState< C >();
  machine.AssertNotInState< D >();
  machine.AssertNotInState< E >();
  machine.AssertNotInState< F >();
  machine.AssertNotInState< G >();
  machine.AssertNotInState< H >();

  machine.process_event( EvToF() );

  machine.AssertInState< A >();
  machine.AssertNotInState< B >();
  machine.AssertInState< C >();
  machine.AssertInState< D >();
  machine.AssertNotInState< E >();
  machine.AssertInState< F >();
  machine.AssertInState< G >();
  machine.AssertNotInState< H >();

  machine.terminate();

  machine.AssertNotInState< A >();
  machine.AssertNotInState< B >();
  machine.AssertNotInState< C >();
  machine.AssertNotInState< D >();
  machine.AssertNotInState< E >();
  machine.AssertNotInState< F >();
  machine.AssertNotInState< G >();
  machine.AssertNotInState< H >();

  return 0;
}
