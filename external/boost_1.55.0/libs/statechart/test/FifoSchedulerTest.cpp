//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/test/test_tools.hpp>

#include <boost/statechart/asynchronous_state_machine.hpp>
#include <boost/statechart/fifo_scheduler.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/termination.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/mpl/list.hpp>

#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <stdexcept>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct EvCheckCtorArgs : sc::event< EvCheckCtorArgs >
{
  public:
    EvCheckCtorArgs( int expectedArgs ) : expectedArgs_( expectedArgs ) {}
    const int expectedArgs_;

  private:
    // avoids C4512 (assignment operator could not be generated)
    EvCheckCtorArgs & operator=( const EvCheckCtorArgs & );
};

struct EvTerminate : sc::event< EvTerminate > {};
struct EvFail : sc::event< EvFail > {};


struct Initial;
struct FifoSchedulerTest :
  sc::asynchronous_state_machine< FifoSchedulerTest, Initial >
{
  public:
    //////////////////////////////////////////////////////////////////////////
    FifoSchedulerTest( my_context ctx ) :
      my_base( ctx ),
      ctorArgs_( 0 )
    {
    }

    FifoSchedulerTest( my_context ctx, int arg1 ) :
      my_base( ctx ),
      ctorArgs_( arg1 )
    {
    }

    FifoSchedulerTest( my_context ctx, int arg1, int arg2 ) :
      my_base( ctx ),
      ctorArgs_( arg1 * 10 + arg2 )
    {
    }

    FifoSchedulerTest( my_context ctx, int arg1, int arg2, int arg3 ) :
      my_base( ctx ),
      ctorArgs_( ( arg1 * 10 + arg2 ) * 10 + arg3 )
    {
    }

    FifoSchedulerTest(
      my_context ctx,
      int arg1, int arg2, int arg3, int arg4
    ) :
      my_base( ctx ),
      ctorArgs_( ( ( arg1 * 10 + arg2 ) * 10 + arg3 ) * 10 + arg4 )
    {
    }

    FifoSchedulerTest(
      my_context ctx,
      int arg1, int arg2, int arg3, int arg4, int arg5
    ) :
      my_base( ctx ),
      ctorArgs_( ( ( ( arg1 * 10 + arg2 ) * 10 +
        arg3 ) * 10 + arg4 ) * 10 + arg5 )
    {
    }

    FifoSchedulerTest(
      my_context ctx,
      int arg1, int arg2, int arg3, int arg4, int arg5, int arg6
    ) :
      my_base( ctx ),
      ctorArgs_( ( ( ( ( arg1 * 10 + arg2 ) * 10 +
        arg3 ) * 10 + arg4 ) * 10 + arg5 ) * 10 + arg6 )
    {
    }

    int CtorArgs()
    {
      return ctorArgs_;
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    const int ctorArgs_;
};

boost::intrusive_ptr< const sc::event_base > MakeEvent(
  const sc::event_base * pEvent )
{
  return boost::intrusive_ptr< const sc::event_base >( pEvent );
}

struct Initial : sc::simple_state< Initial, FifoSchedulerTest >
{
  typedef mpl::list<
    sc::custom_reaction< EvCheckCtorArgs >,
    sc::termination< EvTerminate >,
    sc::custom_reaction< EvFail >
  > reactions;

  sc::result react( const EvCheckCtorArgs & ev )
  {
    BOOST_REQUIRE( ev.expectedArgs_ == outermost_context().CtorArgs() );
    outermost_context_type & machine = outermost_context();
    machine.my_scheduler().queue_event(
      machine.my_handle(), MakeEvent( new EvTerminate() ) );
    return discard_event();
  }

  sc::result react( const EvFail & )
  {
    BOOST_FAIL( "State machine is unexpectedly still running." );
    return discard_event();
  }
};


struct UnexpectedEventCount : public std::runtime_error
{
  UnexpectedEventCount() : std::runtime_error( "" ) {}
};

void RunScheduler(
  sc::fifo_scheduler<> & scheduler, unsigned long expectedEventCount )
{
  // Workaround: For some reason MSVC has a problem with BOOST_REQUIRE here
  // (C1055: compiler limit: out of keys)
  if ( scheduler() != expectedEventCount )
  {
    throw UnexpectedEventCount();
  }
}

static int refArg1;
static int refArg2;
static int refArg3;
static int refArg4;
static int refArg5;
static int refArg6;

void Check(
  sc::fifo_scheduler<> & scheduler,
  const sc::fifo_scheduler<>::processor_handle & processor,
  int ctorArgs )
{
  refArg1 = 6;
  refArg2 = 5;
  refArg3 = 4;
  refArg4 = 3;
  refArg5 = 2;
  refArg6 = 1;

  // Make sure the processor has been created
  RunScheduler( scheduler, 1UL );

  refArg1 = refArg2 = refArg3 = refArg4 = refArg5 = refArg6 = 0;

  scheduler.initiate_processor( processor );
  // This event triggers the queueing of another event, which itself
  // terminates the machine ...
  scheduler.queue_event(
    processor, MakeEvent( new EvCheckCtorArgs( ctorArgs ) ) );
  // ... that's why 3 instead of two events must have been processed
  RunScheduler( scheduler, 3UL );

  // Since the machine has been terminated, this event will be ignored
  scheduler.queue_event( processor, MakeEvent( new EvFail() ) );
  RunScheduler( scheduler, 1UL );

  // Check that we can reinitiate the machine
  scheduler.initiate_processor( processor );
  scheduler.queue_event(
    processor, MakeEvent( new EvCheckCtorArgs( ctorArgs ) ) );
  RunScheduler( scheduler, 3UL );

  // Check that we are terminated again
  scheduler.queue_event( processor, MakeEvent( new EvFail() ) );
  RunScheduler( scheduler, 1UL );

  scheduler.destroy_processor( processor );
  // The following will simply be ignored because the processor has already
  // be destroyed
  scheduler.initiate_processor( processor );
  scheduler.queue_event(
    processor, MakeEvent( new EvCheckCtorArgs( ctorArgs ) ) );
  RunScheduler( scheduler, 3UL );
}

void SetToTrue( bool & value )
{
  value = true;
}

int test_main( int, char* [] )
{
  try
  {
    sc::fifo_scheduler<> scheduler;
    Check( scheduler, scheduler.create_processor< FifoSchedulerTest >(), 0 );

    Check(
      scheduler, scheduler.create_processor< FifoSchedulerTest >( 1 ), 1 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >(
        boost::cref( refArg1 ) ),
      6 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >( 1, 2 ),
      12 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >(
        boost::cref( refArg1 ), boost::cref( refArg2 ) ),
      65 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >( 1, 2, 3 ),
      123 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >(
        boost::cref( refArg1 ), boost::cref( refArg2 ),
        boost::cref( refArg3 ) ),
      654 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >( 1, 2, 3, 4 ),
      1234 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >(
        boost::cref( refArg1 ), boost::cref( refArg2 ),
        boost::cref( refArg3 ), boost::cref( refArg4 ) ),
      6543 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >( 1, 2, 3, 4, 5 ),
      12345 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >(
        boost::cref( refArg1 ), boost::cref( refArg2 ),
        boost::cref( refArg3 ), boost::cref( refArg4 ),
        boost::cref( refArg5 ) ),
      65432 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >( 1, 2, 3, 4, 5, 6 ),
      123456 );

    Check(
      scheduler,
      scheduler.create_processor< FifoSchedulerTest >(
        boost::cref( refArg1 ), boost::cref( refArg2 ),
        boost::cref( refArg3 ), boost::cref( refArg4 ),
        boost::cref( refArg5 ), boost::cref( refArg6 ) ),
      654321 );

    RunScheduler( scheduler, 0UL );
    bool workItem1Processed = false;
    scheduler.queue_work_item(
      boost::bind( &SetToTrue, boost::ref( workItem1Processed ) ) );
    RunScheduler( scheduler, 1UL );
    BOOST_REQUIRE( workItem1Processed );

    scheduler.terminate();
    RunScheduler( scheduler, 1UL );
    BOOST_REQUIRE( scheduler.terminated() );

    RunScheduler( scheduler, 0UL );
    bool workItem2Processed = false;
    scheduler.queue_work_item(
      boost::bind( &SetToTrue, boost::ref( workItem2Processed ) ) );
    // After being terminated, a call to operator() must not process any more
    // events
    RunScheduler( scheduler, 0UL );
    BOOST_REQUIRE( !workItem2Processed );
  }
  catch ( const UnexpectedEventCount & )
  {
    BOOST_FAIL( "Unexpected event count." );
  }

  return 0;
}
