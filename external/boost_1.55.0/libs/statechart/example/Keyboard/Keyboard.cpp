//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// The following example program demonstrates the use of orthogonal states and
// state_downcast to query the state of orthogonal regions.
// Moreover, the use of the state type information interface is also shown.
//////////////////////////////////////////////////////////////////////////////
// #define BOOST_STATECHART_USE_NATIVE_RTTI


#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/mpl/list.hpp>
#include <boost/config.hpp>

#include <iostream>
#include <iomanip>

#ifdef BOOST_INTEL
#  pragma warning( disable: 304 ) // access control not specified
#  pragma warning( disable: 981 ) // operands are evaluated in unspecified order
#endif



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



//////////////////////////////////////////////////////////////////////////////
struct EvNumLockPressed : sc::event< EvNumLockPressed > {};
struct EvCapsLockPressed : sc::event< EvCapsLockPressed > {};
struct EvScrollLockPressed : sc::event< EvScrollLockPressed > {};
struct EvRequestShutdown : sc::event< EvRequestShutdown > {};

struct Active;
struct Keyboard : sc::state_machine< Keyboard, Active > {};

struct NumLockOff;
struct CapsLockOff;
struct ScrollLockOff;
struct Active: sc::simple_state<
  Active, Keyboard, mpl::list< NumLockOff, CapsLockOff, ScrollLockOff > >
{
  typedef sc::custom_reaction< EvRequestShutdown > reactions;

  sc::result react( const EvRequestShutdown & );
};

  struct NumLockOn : sc::simple_state< NumLockOn, Active::orthogonal< 0 > >
  {
    typedef sc::transition< EvNumLockPressed, NumLockOff > reactions;
  };

  struct NumLockOff : sc::simple_state< NumLockOff, Active::orthogonal< 0 > >
  {
    typedef sc::transition< EvNumLockPressed, NumLockOn > reactions;
  };

  struct CapsLockOn : sc::simple_state< CapsLockOn, Active::orthogonal< 1 > >
  {
    typedef sc::transition< EvCapsLockPressed, CapsLockOff > reactions;
  };

  struct CapsLockOff : sc::simple_state< CapsLockOff, Active::orthogonal< 1 > >
  {
    typedef sc::transition< EvCapsLockPressed, CapsLockOn > reactions;
  };

  struct ScrollLockOn : sc::simple_state< ScrollLockOn, Active::orthogonal< 2 > >
  {
    typedef sc::transition< EvScrollLockPressed, ScrollLockOff > reactions;
  };

  struct ScrollLockOff : sc::simple_state< ScrollLockOff, Active::orthogonal< 2 > >
  {
    typedef sc::transition< EvScrollLockPressed, ScrollLockOn > reactions;
  };

sc::result Active::react( const EvRequestShutdown & )
{
  if ( ( state_downcast< const NumLockOff * >() != 0 ) &&
        ( state_downcast< const CapsLockOff * >() != 0 ) &&
        ( state_downcast< const ScrollLockOff * >() != 0 ) )
  {
    std::cout << "Shutdown request accepted\n";
    return terminate();
  }
  else
  {
    std::cout << "Ignoring shutdown request\n\n";
    return discard_event();
  }
}


//////////////////////////////////////////////////////////////////////////////
void DisplayStateConfiguration( const Keyboard & keyboard )
{
  char orthogonalRegion = 'a';

  for ( Keyboard::state_iterator pLeafState = keyboard.state_begin();
    pLeafState != keyboard.state_end(); ++pLeafState )
  {
    std::cout << "Orthogonal region " << orthogonalRegion << ": ";

    const Keyboard::state_base_type * pState = &*pLeafState;

    while ( pState != 0 )
    {
      if ( pState != &*pLeafState )
      {
        std::cout << " -> ";
      }

      #ifdef BOOST_STATECHART_USE_NATIVE_RTTI
      std::cout << std::setw( 15 ) << typeid( *pState ).name();
      #else
      std::cout << std::setw( 15 ) <<
        pState->custom_dynamic_type_ptr< char >();
      #endif
      pState = pState->outer_state_ptr();
    }

    std::cout << "\n";
    ++orthogonalRegion;
  }

  std::cout << "\n";
}


//////////////////////////////////////////////////////////////////////////////
int main()
{
  #ifndef BOOST_STATECHART_USE_NATIVE_RTTI
  Active::custom_static_type_ptr( "Active" );
  NumLockOn::custom_static_type_ptr( "NumLockOn" );
  NumLockOff::custom_static_type_ptr( "NumLockOff" );
  CapsLockOn::custom_static_type_ptr( "CapsLockOn" );
  CapsLockOff::custom_static_type_ptr( "CapsLockOff" );
  ScrollLockOn::custom_static_type_ptr( "ScrollLockOn" );
  ScrollLockOff::custom_static_type_ptr( "ScrollLockOff" );
  #endif

  std::cout << "Boost.Statechart Keyboard example\n\n";
  Keyboard keyboard;
  keyboard.initiate();
  DisplayStateConfiguration( keyboard );
  keyboard.process_event( EvNumLockPressed() );
  DisplayStateConfiguration( keyboard );
  keyboard.process_event( EvRequestShutdown() );
  keyboard.process_event( EvCapsLockPressed() );
  DisplayStateConfiguration( keyboard );
  keyboard.process_event( EvRequestShutdown() );
  keyboard.process_event( EvScrollLockPressed() );
  DisplayStateConfiguration( keyboard );
  keyboard.process_event( EvRequestShutdown() );

  keyboard.process_event( EvNumLockPressed() );
  DisplayStateConfiguration( keyboard );
  keyboard.process_event( EvRequestShutdown() );
  keyboard.process_event( EvCapsLockPressed() );
  DisplayStateConfiguration( keyboard );
  keyboard.process_event( EvRequestShutdown() );
  keyboard.process_event( EvScrollLockPressed() );
  DisplayStateConfiguration( keyboard );
  keyboard.process_event( EvRequestShutdown() );

  return 0;
}
