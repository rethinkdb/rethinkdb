#ifndef BOOST_STATECHART_EXAMPLE_SHOOTING_HPP_INCLUDED
#define BOOST_STATECHART_EXAMPLE_SHOOTING_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "Camera.hpp"

#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/deferral.hpp>

#include <boost/mpl/list.hpp>
#include <boost/config.hpp>

#ifdef BOOST_INTEL
#  pragma warning( disable: 304 ) // access control not specified
#endif



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



//////////////////////////////////////////////////////////////////////////////
struct EvInFocus : sc::event< EvInFocus > {};

struct Focusing;
struct Shooting : sc::simple_state< Shooting, Camera, Focusing >
{
  typedef sc::transition< EvShutterRelease, NotShooting > reactions;

  Shooting();
  ~Shooting();

  void DisplayFocused( const EvInFocus & )
  {
    std::cout << "Focused!\n";
  }
};

  struct Focusing : sc::state< Focusing, Shooting >
  {
    typedef mpl::list<
      sc::custom_reaction< EvInFocus >,
      sc::deferral< EvShutterFull >
    > reactions;

    Focusing( my_context ctx );
    sc::result react( const EvInFocus & );
  };



#endif
