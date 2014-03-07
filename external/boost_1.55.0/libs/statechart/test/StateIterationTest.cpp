//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/mpl/list.hpp>

#include <boost/test/test_tools.hpp>

#include <set>
#include <map>
#include <string>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct EvToA : sc::event< EvToA > {};
struct EvToB : sc::event< EvToB > {};
struct EvToD : sc::event< EvToD > {};
struct EvToE : sc::event< EvToE > {};

struct A;
struct StateIterationTest : sc::state_machine< StateIterationTest, A >
{
  public:
    //////////////////////////////////////////////////////////////////////////
    StateIterationTest();

    void AssertInState( const std::string & stateNames ) const
    {
      stateNamesCache_.clear();

      for ( state_iterator currentState = state_begin();
        currentState != state_end(); ++currentState )
      {
        const StateNamesMap::const_iterator found =
          stateNamesMap_.find( currentState->dynamic_type() );
        BOOST_REQUIRE( found != stateNamesMap_.end() );
        stateNamesCache_.insert( found->second );
      }

      std::string::const_iterator expectedName = stateNames.begin();

      BOOST_REQUIRE( stateNames.size() == stateNamesCache_.size() );

      for ( StateNamesCache::const_iterator actualName =
        stateNamesCache_.begin();
        actualName != stateNamesCache_.end(); ++actualName, ++expectedName )
      {
        BOOST_REQUIRE( ( *actualName )[ 0 ] == *expectedName );
      }
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    typedef std::map< state_base_type::id_type, std::string > StateNamesMap;
    typedef std::set< std::string > StateNamesCache;

    StateNamesMap stateNamesMap_;
    mutable StateNamesCache stateNamesCache_;
};

struct C;
struct D;
struct B : sc::simple_state< B, StateIterationTest, mpl::list< C, D > >
{
  typedef sc::transition< EvToA, A > reactions;
};

struct A : sc::simple_state< A, StateIterationTest >
{
  typedef sc::transition< EvToB, B > reactions;
};

  struct F;
  struct G;
  struct E : sc::simple_state< E, B::orthogonal< 1 >, mpl::list< F, G > >
  {
    typedef sc::transition< EvToD, D > reactions;
  };

    struct F : sc::simple_state< F, E::orthogonal< 0 > > {};
    struct G : sc::simple_state< G, E::orthogonal< 1 > > {};

  struct C : sc::simple_state< C, B::orthogonal< 0 > > {};
  struct D : sc::simple_state< D, B::orthogonal< 1 > >
  {
    typedef sc::transition< EvToE, E > reactions;
  };

StateIterationTest::StateIterationTest()
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


int test_main( int, char* [] )
{
  StateIterationTest machine;
  machine.AssertInState( "" );

  machine.initiate();
  machine.AssertInState( "A" );

  machine.process_event( EvToB() );
  machine.AssertInState( "CD" );

  machine.process_event( EvToA() );
  machine.AssertInState( "A" );

  machine.process_event( EvToB() );
  machine.AssertInState( "CD" );

  machine.process_event( EvToE() );
  machine.AssertInState( "CFG" );

  machine.process_event( EvToD() );
  machine.AssertInState( "CD" );

  machine.process_event( EvToE() );
  machine.AssertInState( "CFG" );

  machine.process_event( EvToA() );
  machine.AssertInState( "A" );

  machine.terminate();
  machine.AssertInState( "" );

  return 0;
}
