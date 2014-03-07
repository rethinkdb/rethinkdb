//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/termination.hpp>

#include <boost/mpl/list.hpp>

#include <boost/test/test_tools.hpp>

#include <set>
#include <map>
#include <string>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct EvTerminateA : sc::event< EvTerminateA > {};
struct EvTerminateB : sc::event< EvTerminateB > {};
struct EvTerminateC : sc::event< EvTerminateC > {};
struct EvTerminateD : sc::event< EvTerminateD > {};
struct EvTerminateE : sc::event< EvTerminateE > {};
struct EvTerminateF : sc::event< EvTerminateF > {};
struct EvTerminateG : sc::event< EvTerminateG > {};

struct A;
struct TerminationTest : sc::state_machine< TerminationTest, A >
{
  public:
    //////////////////////////////////////////////////////////////////////////
    TerminationTest();

    void AssertInState( const std::string & stateNames ) const
    {
      stateNamesCache_.clear();

      for ( state_iterator currentState = state_begin();
        currentState != state_end(); ++currentState )
      {
        AddName( currentState->dynamic_type() );

        const state_base_type * outerState = currentState->outer_state_ptr();

        while ( outerState != 0 )
        {
          AddName( outerState->dynamic_type() );
          outerState = outerState->outer_state_ptr();
        }
      }

      std::string::const_iterator expectedName = stateNames.begin();

      BOOST_REQUIRE( stateNames.size() == stateNamesCache_.size() );

      for ( StateNamesCache::const_iterator actualName =
        stateNamesCache_.begin(); actualName != stateNamesCache_.end();
        ++actualName, ++expectedName )
      {
        BOOST_REQUIRE( ( *actualName )[ 0 ] == *expectedName );
      }
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    void AddName( state_base_type::id_type stateType ) const
    {
      const StateNamesMap::const_iterator found =
        stateNamesMap_.find( stateType );
      BOOST_REQUIRE( found != stateNamesMap_.end() );
      stateNamesCache_.insert( found->second );
    }

    typedef std::map< state_base_type::id_type, std::string > StateNamesMap;
    typedef std::set< std::string > StateNamesCache;

    StateNamesMap stateNamesMap_;
    mutable StateNamesCache stateNamesCache_;
};

template<
  class MostDerived,
  class Context,
  class InnerInitial = mpl::list<> >
struct MyState : sc::simple_state< MostDerived, Context, InnerInitial >
{
  public:
    MyState() : exitCalled_( false ) {}

    ~MyState()
    {
      // BOOST_REQUIRE throws an exception when the test fails. If the state
      // is destructed as part of a stack unwind, abort() is called what is
      // presumably also detected by the test monitor.
      BOOST_REQUIRE( exitCalled_ );
    }

    void exit()
    {
      exitCalled_ = true;
    }

  private:
    bool exitCalled_;
};


struct B;
struct C;
struct A : MyState< A, TerminationTest, mpl::list< B, C > >
{
  typedef sc::termination< EvTerminateA > reactions;
};

  struct B : MyState< B, A::orthogonal< 0 > >
  {
    typedef sc::termination< EvTerminateB > reactions;
  };

  struct D;
  struct E;
  struct C : MyState< C, A::orthogonal< 1 >, mpl::list< D, E > >
  {
    typedef sc::termination< EvTerminateC > reactions;
  };

    struct D : MyState< D, C::orthogonal< 0 > >
    {
      typedef sc::termination< EvTerminateD > reactions;
    };

    struct F;
    struct G;
    struct E : MyState< E, C::orthogonal< 1 >, mpl::list< F, G > >
    {
      typedef sc::termination< EvTerminateE > reactions;
    };

      struct F : MyState< F, E::orthogonal< 0 > >
      {
        typedef sc::termination< EvTerminateF > reactions;
      };

      struct G : MyState< G, E::orthogonal< 1 > >
      {
        typedef sc::termination< EvTerminateG > reactions;
      };

TerminationTest::TerminationTest()
{
  // We're not using custom type information to make this test work even when
  // BOOST_STATECHART_USE_NATIVE_RTTI is defined
  stateNamesMap_[ A::static_type() ] = "A";
  stateNamesMap_[ B::static_type() ] = "B";
  stateNamesMap_[ C::static_type() ] = "C";
  stateNamesMap_[ D::static_type() ] = "D";
  stateNamesMap_[ E::static_type() ] = "E";
  stateNamesMap_[ F::static_type() ] = "F";
  stateNamesMap_[ G::static_type() ] = "G";
}


struct X;
struct TerminationEventBaseTest :
  sc::state_machine< TerminationEventBaseTest, X > {};

struct X : sc::simple_state< X, TerminationEventBaseTest >
{
  typedef sc::termination< sc::event_base > reactions;
};


int test_main( int, char* [] )
{
  TerminationTest machine;
  machine.AssertInState( "" );

  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.process_event( EvTerminateE() );
  machine.AssertInState( "ABCD" );
  machine.process_event( EvTerminateE() );
  machine.AssertInState( "ABCD" );

  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.process_event( EvTerminateC() );
  machine.AssertInState( "AB" );
  machine.process_event( EvTerminateC() );
  machine.AssertInState( "AB" );

  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.process_event( EvTerminateA() );
  machine.AssertInState( "" );
  machine.process_event( EvTerminateA() );
  machine.AssertInState( "" );

  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.process_event( EvTerminateG() );
  machine.AssertInState( "ABCDEF" );
  machine.process_event( EvTerminateG() );
  machine.AssertInState( "ABCDEF" );
  machine.process_event( EvTerminateF() );
  machine.AssertInState( "ABCD" );
  machine.process_event( EvTerminateF() );
  machine.AssertInState( "ABCD" );
  machine.process_event( EvTerminateD() );
  machine.AssertInState( "AB" );
  machine.process_event( EvTerminateD() );
  machine.AssertInState( "AB" );
  machine.process_event( EvTerminateB() );
  machine.AssertInState( "" );
  machine.process_event( EvTerminateB() );
  machine.AssertInState( "" );

  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.process_event( EvTerminateB() );
  machine.AssertInState( "ACDEFG" );
  machine.process_event( EvTerminateB() );
  machine.AssertInState( "ACDEFG" );
  machine.process_event( EvTerminateD() );
  machine.AssertInState( "ACEFG" );
  machine.process_event( EvTerminateD() );
  machine.AssertInState( "ACEFG" );
  machine.process_event( EvTerminateF() );
  machine.AssertInState( "ACEG" );
  machine.process_event( EvTerminateF() );
  machine.AssertInState( "ACEG" );
  machine.process_event( EvTerminateG() );
  machine.AssertInState( "" );
  machine.process_event( EvTerminateG() );
  machine.AssertInState( "" );

  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.process_event( EvTerminateE() );
  machine.AssertInState( "ABCD" );
  machine.process_event( EvTerminateE() );
  machine.AssertInState( "ABCD" );
  machine.process_event( EvTerminateC() );
  machine.AssertInState( "AB" );
  machine.process_event( EvTerminateC() );
  machine.AssertInState( "AB" );
  machine.process_event( EvTerminateA() );
  machine.AssertInState( "" );
  machine.process_event( EvTerminateA() );
  machine.AssertInState( "" );

  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.initiate();
  machine.AssertInState( "ABCDEFG" );
  machine.terminate();
  machine.AssertInState( "" );
  machine.terminate();
  machine.AssertInState( "" );


  TerminationEventBaseTest eventBaseMachine;
  eventBaseMachine.initiate();
  BOOST_REQUIRE( !eventBaseMachine.terminated() );
  eventBaseMachine.process_event( EvTerminateA() );
  BOOST_REQUIRE( eventBaseMachine.terminated() );
  eventBaseMachine.initiate();
  BOOST_REQUIRE( !eventBaseMachine.terminated() );
  eventBaseMachine.process_event( EvTerminateB() );
  BOOST_REQUIRE( eventBaseMachine.terminated() );

  return 0;
}
