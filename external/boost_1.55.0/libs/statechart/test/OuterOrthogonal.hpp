#ifndef BOOST_STATECHART_TEST_OUTER_ORTHOGONAL_HPP_INCLUDED
#define BOOST_STATECHART_TEST_OUTER_ORTHOGONAL_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2004-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state.hpp>
#include <boost/mpl/list.hpp>

#include "InnermostDefault.hpp"



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



//////////////////////////////////////////////////////////////////////////////
template< class MostDerived, class Context, class InitialState0 >
struct Orthogonal0 : sc::state< MostDerived, Context,
  mpl::list<
    InitialState0,
    Default1< MostDerived >,
    Default2< MostDerived > > >
{
  typedef sc::state< 
    MostDerived, Context, mpl::list< InitialState0,
      Default1< MostDerived >, Default2< MostDerived > > > base_type;
  typedef typename base_type::my_context my_context;
  typedef Orthogonal0 my_base;

  Orthogonal0( my_context ctx ) : base_type( ctx )
  {
    this->outermost_context().template ActualEntry< MostDerived >();
  }

  ~Orthogonal0()
  {
    this->outermost_context().template ActualDestructor< MostDerived >();
  }

  void exit()
  {
    this->outermost_context().template ActualExitFunction< MostDerived >();
  }
};

//////////////////////////////////////////////////////////////////////////////
template< class MostDerived, class Context, class InitialState1 >
struct Orthogonal1 : sc::state< MostDerived, Context, 
  mpl::list<
    Default0< MostDerived >,
    InitialState1,
    Default2< MostDerived > > >
{
  typedef sc::state< 
    MostDerived, Context, mpl::list< Default0< MostDerived >,
      InitialState1, Default2< MostDerived > > > base_type;
  typedef typename base_type::my_context my_context;
  typedef Orthogonal1 my_base;

  Orthogonal1( my_context ctx ) : base_type( ctx )
  {
    this->outermost_context().template ActualEntry< MostDerived >();
  }

  ~Orthogonal1()
  {
    this->outermost_context().template ActualDestructor< MostDerived >();
  }

  void exit()
  {
    this->outermost_context().template ActualExitFunction< MostDerived >();
  }
};

//////////////////////////////////////////////////////////////////////////////
template< class MostDerived, class Context, class InitialState2 >
struct Orthogonal2 : sc::state< MostDerived, Context,
  mpl::list<
    Default0< MostDerived >,
    Default1< MostDerived >,
    InitialState2 > >
{
  typedef sc::state< 
    MostDerived, Context, mpl::list< Default0< MostDerived >,
      Default1< MostDerived >, InitialState2 > > base_type;
  typedef typename base_type::my_context my_context;
  typedef Orthogonal2 my_base;

  Orthogonal2( my_context ctx ) : base_type( ctx )
  {
    this->outermost_context().template ActualEntry< MostDerived >();
  }

  ~Orthogonal2()
  {
    this->outermost_context().template ActualDestructor< MostDerived >();
  }

  void exit()
  {
    this->outermost_context().template ActualExitFunction< MostDerived >();
  }
};



#endif
