//////////////////////////////////////////////////////////////////////////////
// Copyright 2004-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/deferral.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/mpl/list.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/test/test_tools.hpp>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



template< class T >
boost::intrusive_ptr< T > MakeIntrusive( T * pT )
{
  return boost::intrusive_ptr< T >( pT );
}

struct EvLeafDeferred : sc::event< EvLeafDeferred > {};
struct EvNodeDeferred : sc::event< EvNodeDeferred > {};
struct EvSwitch : sc::event< EvSwitch > {};
struct EvDestroy : sc::event< EvDestroy > {};

struct Active;
struct DeferralTest : sc::state_machine< DeferralTest, Active >
{
    //////////////////////////////////////////////////////////////////////////
    DeferralTest() : processedCount_( 0 ) {}

    void IncrementProcessedCount()
    {
      ++processedCount_;
    }

    unsigned int ProcessedCount() const
    {
      return processedCount_;
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    unsigned int processedCount_;
};

struct Dead : sc::simple_state< Dead, DeferralTest >
{
  typedef sc::custom_reaction< EvNodeDeferred > reactions;

  sc::result react( const EvNodeDeferred & )
  {
    outermost_context().IncrementProcessedCount();
    return discard_event();
  }
};

struct Idle;
struct Active : sc::simple_state< Active, DeferralTest, Idle >
{
  typedef mpl::list<
    sc::custom_reaction< EvLeafDeferred >,
    sc::deferral< EvNodeDeferred >,
    sc::transition< EvDestroy, Dead >
  > reactions;

  sc::result react( const EvLeafDeferred & )
  {
    outermost_context().IncrementProcessedCount();
    return discard_event();
  }
};

  struct Running : sc::simple_state< Running, Active >
  {
    typedef sc::transition< EvSwitch, Idle > reactions;
  };

  struct Idle : sc::simple_state< Idle, Active >
  {
    typedef mpl::list<
      sc::transition< EvSwitch, Running >,
      sc::deferral< EvLeafDeferred >
    > reactions;
  };


struct EvToX2 : sc::event< EvToX2 > {};
struct EvToX3 : sc::event< EvToX3 > {};

struct X1;
struct DeferralEventBaseTest : sc::state_machine< DeferralEventBaseTest, X1 >
{
};

struct X3 : sc::simple_state< X3, DeferralEventBaseTest > {};

struct X2 : sc::simple_state< X2, DeferralEventBaseTest >
{
  typedef sc::transition< EvToX3, X3 > reactions;
};

struct X1 : sc::simple_state< X1, DeferralEventBaseTest >
{
  typedef mpl::list<
    sc::transition< EvToX2, X2 >,
    sc::deferral< sc::event_base >
  > reactions;
};


int test_main( int, char* [] )
{
  DeferralTest machine;
  machine.initiate();
  machine.process_event( *MakeIntrusive( new EvSwitch() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 0 );
  machine.process_event( *MakeIntrusive( new EvSwitch() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 0 );
  machine.process_event( *MakeIntrusive( new EvLeafDeferred() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 0 );
  machine.process_event( *MakeIntrusive( new EvSwitch() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 1 );
  machine.process_event( *MakeIntrusive( new EvSwitch() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 1 );
  machine.process_event( *MakeIntrusive( new EvLeafDeferred() ) );
  machine.process_event( *MakeIntrusive( new EvLeafDeferred() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 1 );
  machine.process_event( *MakeIntrusive( new EvSwitch() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 3 );
  machine.process_event( *MakeIntrusive( new EvSwitch() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 3 );
  machine.process_event( *MakeIntrusive( new EvNodeDeferred() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 3 );
  machine.process_event( *MakeIntrusive( new EvSwitch() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 3 );
  machine.process_event( EvNodeDeferred() );
  BOOST_REQUIRE( machine.ProcessedCount() == 3 );
  machine.process_event( *MakeIntrusive( new EvDestroy() ) );
  BOOST_REQUIRE( machine.ProcessedCount() == 5 );


  DeferralEventBaseTest eventBaseMachine;
  // state_cast sanity check
  BOOST_REQUIRE_THROW( eventBaseMachine.state_cast< const X1 & >(), std::bad_cast );
  eventBaseMachine.initiate();
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X1 & >() );
  // Deferral must work with heap-allocated and stack-allocated events
  eventBaseMachine.process_event( EvToX3() );
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X1 & >() );
  eventBaseMachine.process_event( EvToX2() );
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X3 & >() );
  eventBaseMachine.initiate();
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X1 & >() );
  eventBaseMachine.process_event( EvToX2() );
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X2 & >() );

  return 0;
}
