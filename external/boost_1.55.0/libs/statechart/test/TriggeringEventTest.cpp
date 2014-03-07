//////////////////////////////////////////////////////////////////////////////
// Copyright 2009 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/exception_translator.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/in_state_reaction.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/mpl/list.hpp>

#include <boost/test/test_tools.hpp>

#include <memory>   // std::allocator



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct EvGoToB : sc::event< EvGoToB > {};
struct EvDoIt : sc::event< EvDoIt > {};

struct A;
struct TriggringEventTest : sc::state_machine<
  TriggringEventTest, A,
  std::allocator< void >, sc::exception_translator<> >
{
  void Transit(const EvGoToB &)
  {
    BOOST_REQUIRE(dynamic_cast<const EvGoToB *>(triggering_event()) != 0);
  }
};

struct B : sc::state< B, TriggringEventTest >
{
  B( my_context ctx ) : my_base( ctx )
  {
    BOOST_REQUIRE(dynamic_cast<const EvGoToB *>(triggering_event()) != 0);
  }

  ~B()
  {
    BOOST_REQUIRE(triggering_event() == 0);
  }

  void DoIt( const EvDoIt & )
  {
    BOOST_REQUIRE(dynamic_cast<const EvDoIt *>(triggering_event()) != 0);
    throw std::exception();
  }

  void HandleException( const sc::exception_thrown & )
  {
    BOOST_REQUIRE(dynamic_cast<const sc::exception_thrown *>(triggering_event()) != 0);
  }

  typedef mpl::list<
    sc::in_state_reaction< EvDoIt, B, &B::DoIt >,
    sc::in_state_reaction< sc::exception_thrown, B, &B::HandleException >
  > reactions;
};

struct A : sc::state< A, TriggringEventTest >
{
  typedef sc::transition<
    EvGoToB, B, TriggringEventTest, &TriggringEventTest::Transit
  > reactions;

  A( my_context ctx ) : my_base( ctx )
  {
    BOOST_REQUIRE(triggering_event() == 0);
  }

  ~A()
  {
    BOOST_REQUIRE(dynamic_cast<const EvGoToB *>(triggering_event()) != 0);
  }
};


int test_main( int, char* [] )
{
  TriggringEventTest machine;
  machine.initiate();
  machine.process_event(EvGoToB());
  machine.process_event(EvDoIt());
  machine.terminate();
  return 0;
}
