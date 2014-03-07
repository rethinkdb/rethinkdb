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
struct UnsupportedDeepHistoryTest : sc::state_machine<
  UnsupportedDeepHistoryTest, A > {};

struct B;
struct A : sc::simple_state<
  A, UnsupportedDeepHistoryTest, B, sc::has_deep_history > {};

  struct C;
  struct D;
  struct B : sc::simple_state< B, A, mpl::list< C, D > > {};

    struct C : sc::simple_state< C, B::orthogonal< 0 > > {};
    struct D : sc::simple_state< D, B::orthogonal< 1 > > {};


int main()
{
  UnsupportedDeepHistoryTest machine;
  machine.initiate();
  return 0;
}
