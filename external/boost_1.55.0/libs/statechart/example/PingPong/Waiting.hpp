#ifndef BOOST_STATECHART_EXAMPLE_WAITING_HPP_INCLUDED
#define BOOST_STATECHART_EXAMPLE_WAITING_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////


#include "Player.hpp"

#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/mpl/list.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



//////////////////////////////////////////////////////////////////////////////
struct Waiting : sc::state< Waiting, Player >
{
  public:
    //////////////////////////////////////////////////////////////////////////
    typedef mpl::list<
      sc::custom_reaction< BallReturned >,
      sc::custom_reaction< GameAborted >
    > reactions;

    Waiting( my_context ctx ) :
      my_base( ctx ),
      noOfReturns_( 0 ),
      pBallReturned_( new BallReturned() )
    {
      outermost_context_type & machine = outermost_context();
      // as we will always return the same event to the opponent, we construct
      // and fill it here so that we can reuse it over and over
      pBallReturned_->returnToOpponent = boost::bind(
        &MyScheduler::queue_event,
        &machine.my_scheduler(), machine.my_handle(), _1 );
      pBallReturned_->abortGame = boost::bind(
        &MyScheduler::queue_event,
        &machine.my_scheduler(), machine.my_handle(),
        MakeIntrusive( new GameAborted() ) );
    }

    sc::result react( const GameAborted & )
    {
      return DestroyMyself();
    }

    sc::result react( const BallReturned & ballReturned )
    {
      outermost_context_type & machine = outermost_context();
      ++machine.TotalNoOfProcessedEvents();

      if ( noOfReturns_++ < machine.GetMaxNoOfReturns() )
      {
        ballReturned.returnToOpponent( pBallReturned_ );
        return discard_event();
      }
      else
      {
        ballReturned.abortGame();
        return DestroyMyself();
      }
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    sc::result DestroyMyself()
    {
      outermost_context_type & machine = outermost_context();
      machine.my_scheduler().destroy_processor( machine.my_handle() );
      machine.my_scheduler().terminate();
      return terminate();
    }

    // avoids C4512 (assignment operator could not be generated)
    Waiting & operator=( const Waiting & );

    unsigned int noOfReturns_;
    const boost::intrusive_ptr< BallReturned > pBallReturned_;
};



#endif
