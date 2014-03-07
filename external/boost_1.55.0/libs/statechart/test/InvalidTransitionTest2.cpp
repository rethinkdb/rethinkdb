//////////////////////////////////////////////////////////////////////////////
// Copyright 2004-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/mpl/list.hpp>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct EvX : sc::event< EvX > {};

struct Active;
struct InvalidTransitionTest : sc::state_machine<
  InvalidTransitionTest, Active > {};

struct Idle0;
struct Idle1;
struct Active : sc::simple_state<
  Active, InvalidTransitionTest, mpl::list< Idle0, Idle1 > > {};

  struct Idle00;
  struct Idle0 : sc::simple_state<
    Idle0, Active::orthogonal< 0 >, Idle00 > {};

    struct Idle00 : sc::simple_state< Idle00, Idle0 > {};

  struct Idle10;
  struct Idle1 : sc::simple_state<
    Idle1, Active::orthogonal< 1 >, Idle10 > {};

    // Invalid transition between different orthogonal regions.
    struct Idle10 : sc::simple_state< Idle10, Idle1 >
    {
      typedef sc::transition< EvX, Idle00 > reactions;
    };


int main()
{
  InvalidTransitionTest machine;
  machine.initiate();
  return 0;
}
