#ifndef BOOST_STATECHART_TEST_TU_TEST_HPP_INCLUDED
#define BOOST_STATECHART_TEST_TU_TEST_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>

#include <boost/config.hpp>

#ifdef BOOST_HAS_DECLSPEC
  #ifdef BOOST_STATECHART_TEST_DYNAMIC_LINK
    #ifdef BOOST_STATECHART_TEST_DLL_EXPORT
      #define BOOST_STATECHART_DECL __declspec(dllexport)
    #else
      #define BOOST_STATECHART_DECL __declspec(dllimport)
    #endif
  #endif
#endif

#ifndef BOOST_STATECHART_DECL
  #define BOOST_STATECHART_DECL
#endif



namespace sc = boost::statechart;

#ifdef BOOST_MSVC
#  pragma warning( push )
   // class X needs to have dll-interface to be used by clients of class Y
#  pragma warning( disable: 4251 )
   // non dll-interface class X used as base for dll-interface class
#  pragma warning( disable: 4275 )
#endif

struct BOOST_STATECHART_DECL EvX : sc::event< EvX > {};
struct BOOST_STATECHART_DECL EvY : sc::event< EvY > {};

struct Initial;
struct BOOST_STATECHART_DECL TuTest : sc::state_machine< TuTest, Initial >
{
  void initiate();
  void unconsumed_event( const sc::event_base & );
};

#ifdef BOOST_MSVC
#  pragma warning( pop )
#endif


#endif
