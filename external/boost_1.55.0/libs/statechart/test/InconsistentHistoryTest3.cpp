//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/deep_history.hpp>

#include <boost/mpl/list.hpp>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct A;
struct InconsistentHistoryTest : sc::state_machine<
  InconsistentHistoryTest, A > {};

struct B;
// A does not have history
struct A : sc::simple_state<
  A, InconsistentHistoryTest, mpl::list< sc::deep_history< B > > > {};

  struct B : sc::simple_state< B, A > {};


int main()
{
  InconsistentHistoryTest machine;
  machine.initiate();
  return 0;
}
