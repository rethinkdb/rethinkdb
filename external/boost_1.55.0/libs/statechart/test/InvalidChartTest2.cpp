//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>



namespace sc = boost::statechart;



struct A;
struct InvalidChartTest : sc::state_machine< InvalidChartTest, A > {};

struct B;
struct A : sc::simple_state< A, InvalidChartTest, B > {};

  struct B : sc::simple_state< B, A > {};

  // A does not have an orthogonal region with the number 1
  struct C : sc::simple_state< C, A::orthogonal< 1 > > {};


int main()
{
  InvalidChartTest machine;
  machine.initiate();
  return 0;
}
