//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "Precompiled.hpp"
#include "Shooting.hpp"
#include <iostream>

#include <boost/config.hpp>

#ifdef BOOST_INTEL
#  pragma warning( disable: 383 ) // reference to temporary used
#endif



//////////////////////////////////////////////////////////////////////////////
Shooting::Shooting()
{
  std::cout << "Entering Shooting\n";
}

Shooting::~Shooting()
{
  std::cout << "Exiting Shooting\n";
}

//////////////////////////////////////////////////////////////////////////////
struct Storing : sc::simple_state< Storing, Shooting >
{
  Storing()
  {
    std::cout << "Picture taken!\n";
  }
};

//////////////////////////////////////////////////////////////////////////////
struct Focused : sc::simple_state< Focused, Shooting >
{
  typedef sc::custom_reaction< EvShutterFull > reactions;

  sc::result react( const EvShutterFull & );
};

sc::result Focused::react( const EvShutterFull & )
{
  if ( context< Camera >().IsMemoryAvailable() )
  {
    return transit< Storing >();
  }
  else
  {
    std::cout << "Cache memory full. Please wait...\n";
    return discard_event();
  }
}

//////////////////////////////////////////////////////////////////////////////
Focusing::Focusing( my_context ctx ) : my_base( ctx )
{
  post_event( boost::intrusive_ptr< EvInFocus >( new EvInFocus() ) );
}

sc::result Focusing::react( const EvInFocus & evt )
{
  return transit< Focused >( &Shooting::DisplayFocused, evt );
}
