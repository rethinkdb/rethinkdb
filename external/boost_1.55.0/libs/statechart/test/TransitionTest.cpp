//////////////////////////////////////////////////////////////////////////////
// Copyright 2004-2007 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include "OuterOrthogonal.hpp"
#include "InnermostDefault.hpp"

#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/null_exception_translator.hpp>
#include <boost/statechart/exception_translator.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/mpl/list.hpp>

#include <boost/test/test_tools.hpp>

#include <typeinfo>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdexcept>



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



typedef std::string ActionDescription();
typedef ActionDescription * ActionDescriptionPtr;
typedef std::vector< ActionDescriptionPtr > ActionDescriptionSequence;
typedef ActionDescriptionSequence::const_iterator SequenceIterator;
typedef void Action( ActionDescriptionSequence & );
typedef Action * ActionPtr;

template< class State >
std::string EntryDescription()
{
  static const std::string entry = "Entry: ";
  return entry + typeid( State ).name() + "\n";
}

template< class State >
std::string ExitFnDescription()
{
  static const std::string exitFunction = "exit(): ";
  return exitFunction + typeid( State ).name() + "\n";
}

template< class State >
std::string DtorDescription()
{
  static const std::string destructor = "Destructor: ";
  return destructor + typeid( State ).name() + "\n";
}

template< class Context, class Event >
std::string TransDescription()
{
  static const std::string transition = "Transition: ";
  static const std::string event = " with Event: ";
  return transition + typeid( Context ).name() +
    event + typeid( Event ).name() + "\n";
}

template< ActionPtr pAction >
std::string ThrowDescription()
{
  static const std::string throwing = "Throwing exception in ";
  ActionDescriptionSequence sequence;
  pAction( sequence );
  return throwing + sequence.front()();
}


template< class State >
void Entry( ActionDescriptionSequence & sequence )
{
  sequence.push_back( &EntryDescription< State > );
}

template< class State >
void ExitFn( ActionDescriptionSequence & sequence )
{
  sequence.push_back( &ExitFnDescription< State > );
}

template< class State >
void Dtor( ActionDescriptionSequence & sequence )
{
  sequence.push_back( &DtorDescription< State > );
}

template< class State >
void Exit( ActionDescriptionSequence & sequence )
{
  ExitFn< State >( sequence );
  Dtor< State >( sequence );
}

template< class Context, class Event >
void Trans( ActionDescriptionSequence & sequence )
{
  sequence.push_back( &TransDescription< Context, Event > );
}

template< ActionPtr pAction >
void Throw( ActionDescriptionSequence & sequence )
{
  sequence.push_back( &ThrowDescription< pAction > );
}

const int arrayLength = 30;
typedef ActionPtr ActionArray[ arrayLength ];


class TransitionTestException : public std::runtime_error
{
  public:
    TransitionTestException() : std::runtime_error( "Oh la la!" ) {}
};


// This test state machine is a beefed-up version of the one presented in
// "Practical Statecharts in C/C++" by Miro Samek, CMP Books 2002
struct A : sc::event< A > {};
struct B : sc::event< B > {};
struct C : sc::event< C > {};
struct D : sc::event< D > {};
struct E : sc::event< E > {};
struct F : sc::event< F > {};
struct G : sc::event< G > {};
struct H : sc::event< H > {};


template< class M > struct S0;
template< class Translator >
struct TransitionTest : sc::state_machine<
  TransitionTest< Translator >, S0< TransitionTest< Translator > >,
  std::allocator< void >, Translator >
{
  public:
    //////////////////////////////////////////////////////////////////////////
    TransitionTest() : pThrowAction_( 0 ), unconsumedEventCount_( 0 ) {}

    ~TransitionTest()
    {
      // Since state destructors access the state machine object, we need to
      // make sure that all states are destructed before this subtype
      // portion is destructed.
      this->terminate();
    }

    void CompareToExpectedActionSequence( ActionArray & actions )
    {
      expectedSequence_.clear();

      // Copy all non-null pointers in actions into expectedSequence_
      for ( ActionPtr * pCurrent = &actions[ 0 ];
        ( pCurrent != &actions[ arrayLength ] ) && ( *pCurrent != 0 );
        ++pCurrent )
      {
        ( *pCurrent )( expectedSequence_ );
      }

      if ( ( expectedSequence_.size() != actualSequence_.size() ) ||
        !std::equal( expectedSequence_.begin(),
          expectedSequence_.end(), actualSequence_.begin() ) )
      {
        std::string message = "\nExpected action sequence:\n";

        for ( SequenceIterator pExpected = expectedSequence_.begin();
          pExpected != expectedSequence_.end(); ++pExpected )
        {
          message += ( *pExpected )();
        }

        message += "\nActual action sequence:\n";

        for ( SequenceIterator pActual = actualSequence_.begin();
          pActual != actualSequence_.end(); ++pActual )
        {
          message += ( *pActual )();
        }

        BOOST_FAIL( message.c_str() );
      }

      actualSequence_.clear();
    }

    void ClearActualSequence()
    {
      actualSequence_.clear();
    }

    void ThrowAction( ActionPtr pThrowAction )
    {
      pThrowAction_ = pThrowAction;
    }

    template< class State >
    void ActualEntry()
    {
      StoreActualAction< &Entry< State > >();
    }

    template< class State >
    void ActualExitFunction()
    {
      StoreActualAction< &ExitFn< State > >();
    }
    
    template< class State >
    void ActualDestructor()
    {
      StoreActualAction< &Dtor< State > >();
    }
    
    template< class Context, class Event >
    void ActualTransition()
    {
      StoreActualAction< &Trans< Context, Event > >();
    }

    void unconsumed_event( const sc::event_base & )
    {
      ++unconsumedEventCount_;
    }

    unsigned int GetUnconsumedEventCount() const
    {
      return unconsumedEventCount_;
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    template< ActionPtr pAction >
    void StoreActualAction()
    {
      if ( pAction == pThrowAction_ )
      {
        Throw< pAction >( actualSequence_ );
        throw TransitionTestException();
      }
      else
      {
        pAction( actualSequence_ );
      }
    }

    ActionPtr pThrowAction_;
    ActionDescriptionSequence actualSequence_;
    ActionDescriptionSequence expectedSequence_;
    unsigned int unconsumedEventCount_;
};

template< class M > struct S1;
template< class M > struct S211;
template< class M >
struct S0 : Orthogonal0< S0< M >, M, S1< M > >
{
  typedef Orthogonal0< S0< M >, M, S1< M > > my_base;
  public:
    typedef sc::transition< E, S211< M > > reactions;

    S0( typename my_base::my_context ctx ) : my_base( ctx ) {}

    void Transit( const A & evt ) { TransitImpl( evt ); }
    void Transit( const B & evt ) { TransitImpl( evt ); }
    void Transit( const C & evt ) { TransitImpl( evt ); }
    void Transit( const D & evt ) { TransitImpl( evt ); }
    void Transit( const F & evt ) { TransitImpl( evt ); }
    void Transit( const G & evt ) { TransitImpl( evt ); }
    void Transit( const H & evt ) { TransitImpl( evt ); }

  private:
    template< class Event >
    void TransitImpl( const Event & )
    {
      this->outermost_context().template ActualTransition< S0< M >, Event >();
    }
};

  template< class M > struct S11;
  template< class M > struct S21;
  template< class M >
  struct S2 : Orthogonal2< S2< M >, S0< M >, S21< M > >
  {
    typedef Orthogonal2< S2< M >, S0< M >, S21< M > > my_base;
    typedef mpl::list<
      sc::transition< C, S1< M >, S0< M >, &S0< M >::Transit >,
      sc::transition< F, S11< M >, S0< M >, &S0< M >::Transit >
    > reactions;

    S2( typename my_base::my_context ctx ) : my_base( ctx ) {}
  };

    template< class M >
    struct S21 : Orthogonal1<
      S21< M >, typename S2< M >::template orthogonal< 2 >, S211< M > >
    {
      typedef Orthogonal1<
        S21< M >, typename S2< M >::template orthogonal< 2 >, S211< M >
      > my_base;
      typedef mpl::list<
        sc::transition< H, S21< M >, S0< M >, &S0< M >::Transit >,
        sc::transition< B, S211< M >, S0< M >, &S0< M >::Transit >
      > reactions;

      S21( typename my_base::my_context ctx ) : my_base( ctx ) {}
    };

      template< class M >
      struct S211 : InnermostDefault<
        S211< M >, typename S21< M >::template orthogonal< 1 > >
      {
        typedef InnermostDefault<
          S211< M >, typename S21< M >::template orthogonal< 1 > > my_base;
        typedef mpl::list<
          sc::transition< D, S21< M >, S0< M >, &S0< M >::Transit >,
          sc::transition< G, S0< M > >
        > reactions;

        S211( typename my_base::my_context ctx ) : my_base( ctx ) {}
      };

  template< class M >
  struct S1 : Orthogonal1< S1< M >, S0< M >, S11< M > >
  {
    typedef Orthogonal1< S1< M >, S0< M >, S11< M > > my_base;
    typedef mpl::list<
      sc::transition< A, S1< M >, S0< M >, &S0< M >::Transit >,
      sc::transition< B, S11< M >, S0< M >, &S0< M >::Transit >,
      sc::transition< C, S2< M >, S0< M >, &S0< M >::Transit >,
      sc::transition< D, S0< M > >,
      sc::transition< F, S211< M >, S0< M >, &S0< M >::Transit >
    > reactions;

    S1( typename my_base::my_context ctx ) : my_base( ctx ) {}
  };

    template< class M >
    struct S11 : InnermostDefault<
      S11< M >, typename S1< M >::template orthogonal< 1 > >
    {
      typedef InnermostDefault<
        S11< M >, typename S1< M >::template orthogonal< 1 > > my_base;
      typedef mpl::list<
        sc::transition< G, S211< M >, S0< M >, &S0< M >::Transit >,
        sc::custom_reaction< H >
      > reactions;

      S11( typename my_base::my_context ctx ) : my_base( ctx ) {}

      sc::result react( const H & )
      {
        this->outermost_context().template ActualTransition< S11< M >, H >();
        return this->discard_event();
      }
    };


struct X1;
struct TransitionEventBaseTest :
  sc::state_machine< TransitionEventBaseTest, X1 >
{
  public:
    TransitionEventBaseTest() : actionCallCounter_( 0 ) {}

    void Transit( const sc::event_base & eventBase )
    {
      BOOST_REQUIRE(
        ( dynamic_cast< const B * >( &eventBase ) != 0 ) ||
        ( dynamic_cast< const D * >( &eventBase ) != 0 ) );
      ++actionCallCounter_;
    }

    unsigned int GetActionCallCounter() const
    {
      return actionCallCounter_;
    }

  private:
    unsigned int actionCallCounter_;
};

struct X2 : sc::simple_state< X2, TransitionEventBaseTest >
{
  typedef sc::transition< sc::event_base, X1,
    TransitionEventBaseTest, &TransitionEventBaseTest::Transit > reactions;
};

struct X1 : sc::simple_state< X1, TransitionEventBaseTest >
{
  typedef sc::transition< sc::event_base, X2 > reactions;
};

template< class M >
void TestTransitions( M & machine )
{
  machine.initiate();
  ActionArray init = 
  {
    Entry< S0< M > >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >
  };
  machine.CompareToExpectedActionSequence( init );

  machine.process_event( A() );
  ActionArray a1 =
  {
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Trans< S0< M >, A >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >
  };
  machine.CompareToExpectedActionSequence( a1 );

  machine.process_event( B() );
  ActionArray b1 =
  {
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Trans< S0< M >, B >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >
  };
  machine.CompareToExpectedActionSequence( b1 );

  machine.process_event( C() );
  ActionArray c1 =
  {
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Trans< S0< M >, C >,
    Entry< S2< M > >,
    Entry< Default0< S2< M > > >,
    Entry< Default1< S2< M > > >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >
  };
  machine.CompareToExpectedActionSequence( c1 );

  machine.process_event( D() );
  ActionArray d2 =
  {
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Trans< S0< M >, D >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >
  };
  machine.CompareToExpectedActionSequence( d2 );

  machine.process_event( E() );
  ActionArray e2 =
  {
    Exit< Default2< S0< M > > >,
    Exit< Default1< S0< M > > >,
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Exit< Default1< S2< M > > >,
    Exit< Default0< S2< M > > >,
    Exit< S2< M > >,
    Exit< S0< M > >,
    Entry< S0< M > >,
    Entry< S2< M > >,
    Entry< Default0< S2< M > > >,
    Entry< Default1< S2< M > > >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >
  };
  machine.CompareToExpectedActionSequence( e2 );

  machine.process_event( F() );
  ActionArray f2 =
  {
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Exit< Default1< S2< M > > >,
    Exit< Default0< S2< M > > >,
    Exit< S2< M > >,
    Trans< S0< M >, F >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >
  };
  machine.CompareToExpectedActionSequence( f2 );

  machine.process_event( G() );
  ActionArray g1 =
  {
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Trans< S0< M >, G >,
    Entry< S2< M > >,
    Entry< Default0< S2< M > > >,
    Entry< Default1< S2< M > > >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >
  };
  machine.CompareToExpectedActionSequence( g1 );

  machine.process_event( H() );
  ActionArray h2 =
  {
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Trans< S0< M >, H >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >
  };
  machine.CompareToExpectedActionSequence( h2 );

  BOOST_REQUIRE( machine.GetUnconsumedEventCount() == 0 );
  machine.process_event( A() );
  BOOST_REQUIRE( machine.GetUnconsumedEventCount() == 1 );
  ActionArray a2 =
  {
  };
  machine.CompareToExpectedActionSequence( a2 );

  machine.process_event( B() );
  ActionArray b2 =
  {
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Trans< S0< M >, B >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >
  };
  machine.CompareToExpectedActionSequence( b2 );

  machine.process_event( C() );
  ActionArray c2 =
  {
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Exit< Default1< S2< M > > >,
    Exit< Default0< S2< M > > >,
    Exit< S2< M > >,
    Trans< S0< M >, C >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >
  };
  machine.CompareToExpectedActionSequence( c2 );

  machine.process_event( D() );
  ActionArray d1 = 
  {
    Exit< Default2< S0< M > > >,
    Exit< Default1< S0< M > > >,
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Exit< S0< M > >,
    Entry< S0< M > >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >
  };
  machine.CompareToExpectedActionSequence( d1 );

  machine.process_event( F() );
  ActionArray f1 =
  {
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Trans< S0< M >, F >,
    Entry< S2< M > >,
    Entry< Default0< S2< M > > >,
    Entry< Default1< S2< M > > >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >
  };
  machine.CompareToExpectedActionSequence( f1 );

  machine.process_event( G() );
  ActionArray g2 =
  {
    Exit< Default2< S0< M > > >,
    Exit< Default1< S0< M > > >,
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Exit< Default1< S2< M > > >,
    Exit< Default0< S2< M > > >,
    Exit< S2< M > >,
    Exit< S0< M > >,
    Entry< S0< M > >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >
  };
  machine.CompareToExpectedActionSequence( g2 );

  machine.process_event( H() );
  ActionArray h1 =
  {
    Trans< S11< M >, H >
  };
  machine.CompareToExpectedActionSequence( h1 );

  machine.process_event( E() );
  ActionArray e1 =
  {
    Exit< Default2< S0< M > > >,
    Exit< Default1< S0< M > > >,
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Exit< S0< M > >,
    Entry< S0< M > >,
    Entry< S2< M > >,
    Entry< Default0< S2< M > > >,
    Entry< Default1< S2< M > > >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    Entry< S211< M > >,
    Entry< Default2< S21< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >
  };
  machine.CompareToExpectedActionSequence( e1 );

  machine.terminate();
  ActionArray term =
  {
    Exit< Default2< S0< M > > >,
    Exit< Default1< S0< M > > >,
    Exit< Default2< S21< M > > >,
    Exit< S211< M > >,
    Exit< Default0< S21< M > > >,
    Exit< S21< M > >,
    Exit< Default1< S2< M > > >,
    Exit< Default0< S2< M > > >,
    Exit< S2< M > >,
    Exit< S0< M > >
  };
  machine.CompareToExpectedActionSequence( term );

  machine.ThrowAction( &Entry< Default0< S1< M > > > );
  BOOST_REQUIRE_THROW( machine.initiate(), TransitionTestException );
  ActionArray initThrow1 = 
  {
    Entry< S0< M > >,
    Entry< S1< M > >,
    &::Throw< &::Entry< Default0< S1< M > > > >,
    Dtor< S1< M > >,
    Dtor< S0< M > >
  };
  machine.CompareToExpectedActionSequence( initThrow1 );
  BOOST_REQUIRE( machine.terminated() );

  machine.ThrowAction( &Entry< S11< M > > );
  BOOST_REQUIRE_THROW( machine.initiate(), TransitionTestException );
  ActionArray initThrow2 = 
  {
    Entry< S0< M > >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    &::Throw< &::Entry< S11< M > > >,
    Dtor< Default0< S1< M > > >,
    Dtor< S1< M > >,
    Dtor< S0< M > >
  };
  machine.CompareToExpectedActionSequence( initThrow2 );
  BOOST_REQUIRE( machine.terminated() );

  machine.ThrowAction( &Trans< S0< M >, A > );
  machine.initiate();
  BOOST_REQUIRE_THROW( machine.process_event( A() ), TransitionTestException );
  ActionArray a1Throw1 =
  {
    Entry< S0< M > >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >,
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    &::Throw< &::Trans< S0< M >, A > >,
    Dtor< Default2< S0< M > > >,
    Dtor< Default1< S0< M > > >,
    Dtor< S0< M > >
  };
  machine.CompareToExpectedActionSequence( a1Throw1 );
  BOOST_REQUIRE( machine.terminated() );

  machine.ThrowAction( &Entry< S211< M > > );
  machine.initiate();
  BOOST_REQUIRE_THROW( machine.process_event( C() ), TransitionTestException );
  ActionArray c1Throw1 =
  {
    Entry< S0< M > >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >,
    Exit< Default2< S1< M > > >,
    Exit< S11< M > >,
    Exit< Default0< S1< M > > >,
    Exit< S1< M > >,
    Trans< S0< M >, C >,
    Entry< S2< M > >,
    Entry< Default0< S2< M > > >,
    Entry< Default1< S2< M > > >,
    Entry< S21< M > >,
    Entry< Default0< S21< M > > >,
    &::Throw< &::Entry< S211< M > > >,
    Dtor< Default2< S0< M > > >,
    Dtor< Default1< S0< M > > >,
    Dtor< Default0< S21< M > > >,
    Dtor< S21< M > >,
    Dtor< Default1< S2< M > > >,
    Dtor< Default0< S2< M > > >,
    Dtor< S2< M > >,
    Dtor< S0< M > >
  };
  machine.CompareToExpectedActionSequence( c1Throw1 );
  BOOST_REQUIRE( machine.terminated() );

  machine.ThrowAction( &ExitFn< S11< M > > );
  machine.initiate();
  BOOST_REQUIRE_THROW( machine.process_event( C() ), TransitionTestException );
  ActionArray c1Throw2 =
  {
    Entry< S0< M > >,
    Entry< S1< M > >,
    Entry< Default0< S1< M > > >,
    Entry< S11< M > >,
    Entry< Default2< S1< M > > >,
    Entry< Default1< S0< M > > >,
    Entry< Default2< S0< M > > >,
    Exit< Default2< S1< M > > >,
    &::Throw< &::ExitFn< S11< M > > >,
    Dtor< S11< M > >,
    Dtor< Default2< S0< M > > >,
    Dtor< Default1< S0< M > > >,
    Dtor< Default0< S1< M > > >,
    Dtor< S1< M > >,
    Dtor< S0< M > >
  };
  machine.CompareToExpectedActionSequence( c1Throw2 );
  BOOST_REQUIRE( machine.terminated() );
  BOOST_REQUIRE( machine.GetUnconsumedEventCount() == 1 );
}


int test_main( int, char* [] )
{
  TransitionTest< sc::null_exception_translator > null_machine;
  TestTransitions( null_machine );
  TransitionTest< sc::exception_translator<> > machine;
  TestTransitions( machine );

  TransitionEventBaseTest eventBaseMachine;
  eventBaseMachine.initiate();
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X1 & >() );
  eventBaseMachine.process_event( A() );
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X2 & >() );
  BOOST_REQUIRE( eventBaseMachine.GetActionCallCounter() == 0 );
  eventBaseMachine.process_event( B() );
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X1 & >() );
  BOOST_REQUIRE( eventBaseMachine.GetActionCallCounter() == 1 );
  eventBaseMachine.process_event( C() );
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X2 & >() );
  BOOST_REQUIRE( eventBaseMachine.GetActionCallCounter() == 1 );
  eventBaseMachine.process_event( D() );
  BOOST_REQUIRE_NO_THROW( eventBaseMachine.state_cast< const X1 & >() );
  BOOST_REQUIRE( eventBaseMachine.GetActionCallCounter() == 2 );

  return 0;
}
