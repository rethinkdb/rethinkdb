//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/deep_history.hpp>
#include <boost/statechart/transition.hpp>



namespace sc = boost::statechart;



struct EvX : sc::event< EvX > {};

struct A;
struct InconsistentHistoryTest : sc::state_machine<
  InconsistentHistoryTest, A > {};

struct B;
// A only has shallow history
struct A : sc::simple_state<
  A, InconsistentHistoryTest, B, sc::has_shallow_history >
{
  typedef sc::transition< EvX, sc::deep_history< B > > reactions;
};

  struct B : sc::simple_state< B, A > {};


int main()
{
  InconsistentHistoryTest machine;
  machine.initiate();
  return 0;
}
