//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>

#include <boost/mpl/list.hpp>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct A;
struct InvalidChartTest : sc::state_machine< InvalidChartTest, A > {};
struct B;
struct C;
struct A : sc::simple_state< A, InvalidChartTest, mpl::list< B, C > > {};

// B resides in the 0th region not the 1st
struct B : sc::simple_state< B, A::orthogonal< 1 > > {};
struct C : sc::simple_state< C, A::orthogonal< 1 > > {};

int main()
{
  InvalidChartTest machine;
  machine.initiate();
  return 0;
}
