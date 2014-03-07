#ifndef BOOST_STATECHART_EXAMPLE_PLAYER_HPP_INCLUDED
#define BOOST_STATECHART_EXAMPLE_PLAYER_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event.hpp>
#include <boost/statechart/fifo_scheduler.hpp>
#include <boost/statechart/asynchronous_state_machine.hpp>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/mpl/list.hpp>
#include <boost/function.hpp>

#ifdef CUSTOMIZE_MEMORY_MANAGEMENT
#  ifdef BOOST_HAS_THREADS
     // for some reason the following is not automatically defined
#    if defined( BOOST_MSVC ) | defined( BOOST_INTEL )
#      define __WIN32__
#    endif
#  else
#    define BOOST_NO_MT
#  endif

#  ifdef BOOST_MSVC
#    pragma warning( push )
#    pragma warning( disable: 4127 ) // conditional expression is constant
#  endif

#  include <boost/pool/pool_alloc.hpp>

#  ifdef BOOST_MSVC
#    pragma warning( pop )
#  endif
#endif

#include <memory> // std::allocator



namespace sc = boost::statechart;



//////////////////////////////////////////////////////////////////////////////
template< class T >
boost::intrusive_ptr< T > MakeIntrusive( T * pObject )
{
  return boost::intrusive_ptr< T >( pObject );
}


//////////////////////////////////////////////////////////////////////////////
struct BallReturned : sc::event< BallReturned >
{
  boost::function1< void, const boost::intrusive_ptr< const BallReturned > & >
    returnToOpponent;
  boost::function0< void > abortGame;
};

struct GameAborted : sc::event< GameAborted > {};

#ifdef CUSTOMIZE_MEMORY_MANAGEMENT
typedef boost::fast_pool_allocator< int > MyAllocator;
typedef sc::fifo_scheduler< 
  sc::fifo_worker< MyAllocator >, MyAllocator > MyScheduler;
#else
typedef std::allocator< void > MyAllocator;
typedef sc::fifo_scheduler<> MyScheduler;
#endif


//////////////////////////////////////////////////////////////////////////////
struct Player;
struct Waiting;

namespace boost
{
namespace statechart
{
  // The following class member specialization ensures that
  // state_machine<>::initiate is not instantiated at a point where Waiting
  // is not defined yet.
  template<>
  inline void asynchronous_state_machine<
    Player, Waiting, MyScheduler, MyAllocator >::initiate_impl() {}
}
}


struct Player : sc::asynchronous_state_machine<
  Player, Waiting, MyScheduler, MyAllocator >
{
  public:
    Player( my_context ctx, unsigned int maxNoOfReturns ) :
      my_base( ctx ),
      maxNoOfReturns_( maxNoOfReturns )
    {
    }

    static unsigned int & TotalNoOfProcessedEvents()
    {
      return totalNoOfProcessedEvents_;
    }

    unsigned int GetMaxNoOfReturns() const
    {
      return maxNoOfReturns_;
    }

  private:
    // This function is defined in the Player.cpp
    virtual void initiate_impl();

    static unsigned int totalNoOfProcessedEvents_;
    const unsigned int maxNoOfReturns_;
};



#endif
