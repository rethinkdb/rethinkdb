//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "TuTest.hpp"

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/in_state_reaction.hpp>

#include <stdexcept>



struct Initial : sc::simple_state< Initial, TuTest >
{
  void Whatever( const EvX & ) {}

  typedef sc::in_state_reaction< EvX, Initial, &Initial::Whatever > reactions;
};



void TuTest::initiate()
{
  sc::state_machine< TuTest, Initial >::initiate();
}

void TuTest::unconsumed_event( const sc::event_base & )
{
  throw std::runtime_error( "Event was not consumed!" );
}
