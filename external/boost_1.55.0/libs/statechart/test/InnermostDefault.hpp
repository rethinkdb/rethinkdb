#ifndef BOOST_STATECHART_TEST_INNERMOST_DEFAULT_HPP_INCLUDED
#define BOOST_STATECHART_TEST_INNERMOST_DEFAULT_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2004-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state.hpp>



namespace sc = boost::statechart;



//////////////////////////////////////////////////////////////////////////////
template< class MostDerived, class Context >
struct InnermostDefault : sc::state< MostDerived, Context >
{
  typedef sc::state< MostDerived, Context > base_type;
  typedef typename base_type::my_context my_context;
  typedef InnermostDefault my_base;

  InnermostDefault( my_context ctx ) : base_type( ctx )
  {
    this->outermost_context().template ActualEntry< MostDerived >();
  }

  ~InnermostDefault()
  {
    this->outermost_context().template ActualDestructor< MostDerived >();
  }

  void exit()
  {
    this->outermost_context().template ActualExitFunction< MostDerived >();
  }
};

//////////////////////////////////////////////////////////////////////////////
template< class Context >
struct Default0 : InnermostDefault<
  Default0< Context >, typename Context::template orthogonal< 0 > >
{
  typedef InnermostDefault<
    Default0, typename Context::template orthogonal< 0 > > base_type;
  typedef typename base_type::my_context my_context;

  Default0( my_context ctx ) : base_type( ctx ) {}
};

//////////////////////////////////////////////////////////////////////////////
template< class Context >
struct Default1 : InnermostDefault<
  Default1< Context >, typename Context::template orthogonal< 1 > >
{
  typedef InnermostDefault<
    Default1, typename Context::template orthogonal< 1 > > base_type;
  typedef typename base_type::my_context my_context;

  Default1( my_context ctx ) : base_type( ctx ) {}
};

//////////////////////////////////////////////////////////////////////////////
template< class Context >
struct Default2 : InnermostDefault<
  Default2< Context >, typename Context::template orthogonal< 2 > >
{
  typedef InnermostDefault<
    Default2, typename Context::template orthogonal< 2 > > base_type;
  typedef typename base_type::my_context my_context;

  Default2( my_context ctx ) : base_type( ctx ) {}
};



#endif
