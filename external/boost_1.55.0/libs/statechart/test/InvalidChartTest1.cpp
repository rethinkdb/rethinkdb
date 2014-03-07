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

struct A : sc::simple_state< A, InvalidChartTest > {};

  // A does not have inner states
  struct B : sc::simple_state< B, A > {};


int main()
{
  InvalidChartTest machine;
  machine.initiate();
  return 0;
}
