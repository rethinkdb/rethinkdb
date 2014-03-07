//////////////////////////////////////////////////////////////////////////////
// Copyright 2010 Igor R (http://thread.gmane.org/gmane.comp.lib.boost.user/62985)
// Copyright 2010 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////

#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/deferral.hpp>

#include <boost/mpl/list.hpp>

#include <boost/test/test_tools.hpp>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct ev1to2 : sc::event< ev1to2 > {};
struct ev2to3 : sc::event< ev2to3 > {};
struct ev3to4_1 : sc::event< ev3to4_1 > {};
struct ev3to4_2 : sc::event< ev3to4_2 > {};

struct s1;
struct fsm : sc::state_machine< fsm, s1 > {};

struct s2;
struct s1 : sc::simple_state< s1, fsm >
{
  typedef mpl::list<
    sc::transition< ev1to2, s2 >,
    sc::deferral< ev2to3 >,
    sc::deferral< ev3to4_1 >,
    sc::deferral< ev3to4_2 >
  > reactions;
};

struct s3;
struct s2 : sc::simple_state< s2, fsm >
{
  typedef mpl::list<
    sc::transition< ev2to3, s3 >,
    sc::deferral< ev3to4_1 >,
    sc::deferral< ev3to4_2 >
  > reactions;
};

struct s4_1;
struct s4_2;
struct s3 : sc::simple_state< s3, fsm >
{
  typedef mpl::list<
    sc::transition< ev3to4_1, s4_1 >,
    sc::transition< ev3to4_2, s4_2 >
  > reactions;
};

struct s4_1 : sc::simple_state< s4_1, fsm > {};
struct s4_2 : sc::simple_state< s4_2, fsm > {};

int test_main( int, char* [] )
{
  fsm machine;
  machine.initiate();
  machine.process_event( ev3to4_1() );
  machine.process_event( ev2to3() );
  machine.process_event( ev3to4_2() );
  machine.process_event( ev1to2() );
  BOOST_REQUIRE( machine.state_cast< const s4_1 * >() != 0 );

  return 0;
}
