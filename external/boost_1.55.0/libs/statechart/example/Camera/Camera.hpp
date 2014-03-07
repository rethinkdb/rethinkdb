#ifndef BOOST_STATECHART_EXAMPLE_CAMERA_HPP_INCLUDED
#define BOOST_STATECHART_EXAMPLE_CAMERA_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/config.hpp>

#ifdef BOOST_INTEL
#  pragma warning( disable: 304 ) // access control not specified
#endif



namespace sc = boost::statechart;



//////////////////////////////////////////////////////////////////////////////
struct EvShutterHalf : sc::event< EvShutterHalf > {};
struct EvShutterFull : sc::event< EvShutterFull > {};
struct EvShutterRelease : sc::event< EvShutterRelease > {};
struct EvConfig : sc::event< EvConfig > {};

struct NotShooting;
struct Camera : sc::state_machine< Camera, NotShooting >
{
    bool IsMemoryAvailable() const { return true; }
    bool IsBatteryLow() const { return false; }
};

struct Idle;
struct NotShooting : sc::simple_state< NotShooting, Camera, Idle >
{
  typedef sc::custom_reaction< EvShutterHalf > reactions;

  NotShooting();
  ~NotShooting();

  sc::result react( const EvShutterHalf & );
};

  struct Idle : sc::simple_state< Idle, NotShooting >
  {
    typedef sc::custom_reaction< EvConfig > reactions;

    Idle();
    ~Idle();

    sc::result react( const EvConfig & );
  };



#endif
