#ifndef BOOST_STATECHART_EXAMPLE_CONFIGURING_HPP_INCLUDED
#define BOOST_STATECHART_EXAMPLE_CONFIGURING_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "Camera.hpp"

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/config.hpp>

#ifdef BOOST_INTEL
#  pragma warning( disable: 304 ) // access control not specified
#endif



namespace sc = boost::statechart;



//////////////////////////////////////////////////////////////////////////////
struct Configuring : sc::simple_state< Configuring, NotShooting >
{
  typedef sc::transition< EvConfig, Idle > reactions;

  Configuring();
  ~Configuring();
};



#endif
